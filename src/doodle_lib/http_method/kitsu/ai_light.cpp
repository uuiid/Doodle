//
// AI 灯光：HTTP 处理函数
//
// 每个处理函数只做四件事：权限 → 校验入参 → 调编排层 → 组响应。
// 状态推进与业务规则一律在 ai_light_orchestrator.cpp，这里不写状态机。
//
// 服务端不持有提示词模板与 schema 真源，也不替前端跑推理 —— 三条 LLM 接缝
// （generate_spec / generate_candidates / structure_feedback）均已归前端，
// 见 docs/ue_ai_lighting_role_downgrade.md。
//
// ⚠️ Spec 的提交入口目前不存在（`/spec` POST 已随第三梯队删除，退化成空写），
// 是本方案唯一未闭合的缺口，见该文档 §3.3。
//

#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/ai_studio.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/preview_file.h>
// 推理任务行（seedance2::task）与它的落库/计费入口。
#include <doodle_core/metadata/seedance2/task.h>
#include <doodle_core/metadata/server_task_info.h>
// 必须显式包含：get_tasks_for_entity_and_task_type() 返回 std::optional<task>，
// 而 task_status.h 只给了 task 的前置声明，实例化 optional<task> 需要完整类型。
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
#include <doodle_lib/http_method/kitsu/preview.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/beast/http/status.hpp>

#include <algorithm>
#include <magic_enum/magic_enum.hpp>
#include <opencv2/opencv.hpp>
#include <string>
#include <vector>

namespace doodle::http::ai_light {

namespace {

using namespace orm;
// 表实体与编排函数都在 doodle::ai_light 里。这里可以放心 using ——
// 路由类都带 _route 后缀，不会遮蔽任何实体名（见 ai_light.h 顶部的说明）。
using namespace doodle::ai_light;

/// 查会话并做项目级权限校验。返回会话本体。
///
/// 为什么每次都要回查 project：路由上的 {id} 是会话 id，而权限判定要的是项目 id，
/// 会话行是唯一权威来源。让前端传 project_id 来「帮忙」只会引入一处可伪造的输入。
///
/// 会话不存在时 get_by_uuid → to_single 会抛 sqlite_orm_exception{404}，
/// 由 http_session_data 统一翻成 404，这里不再自己 try/catch。
ai_light_session load_session_checked(
    const http_jwt_fun::http_jwt_t& in_person, sqlite_database& in_sql, const uuid& in_session_id
) {
  auto l_session = in_sql.get_by_uuid<ai_light_session>(in_session_id);
  in_person.check_project_access(l_session.project_id_);
  return l_session;
}

/// 从镜头解析出本次会话归属的 Kitsu 任务 —— 即该镜头的**灯光任务**。
///
/// 为什么按 task_type id 精确匹配，而不是「按任务类型优先级挑一个」：
/// `task_type::priority_` 是**预览面板的展示顺序**（灯光/动画/合成…），
/// 拿它当「哪个任务是这次灯光工作的」是两件不同的事 —— 只要动画的优先级更高，
/// 定稿图就会被挂到动画任务下。而 `get_preview_files_for_entity` 是按 task_id 分组的，
/// 挂错组**不会报错**，只会静静地出现在错误的分类里，属于最难发现的一类问题。
/// 灯光任务类型的 id 在本工程是常量（task_type.h:62），比 id 既准确又不依赖名称文本（可本地化）。
///
/// 找不到时**必须失败**，不能退化成 nil：`preview_file.task_id_` 是指向 task 的外键，
/// 且 `get_preview_files_for_entity` 用 inner join 关联 task，
/// nil 会让定稿图永远进不了镜头的预览面板 —— 那是「看起来成功、实际丢结果」。
uuid resolve_lighting_task_id(sqlite_database& in_sql, const uuid& in_entity_id) {
  auto l_task = in_sql.get_tasks_for_entity_and_task_type(in_entity_id, task_type::get_lighting_id());
  if (!l_task.has_value())
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request,
            fmt::format("镜头 {} 没有灯光任务，无法建立 AI 灯光会话", in_entity_id)
        }
    );
  return l_task->uuid_id_;
}

bool query_bool(const boost::url& in_url, std::string_view in_key, bool in_default) {
  for (auto&& [l_key, l_value, l_has] : in_url.params()) {
    if (l_key == in_key && l_has) return l_value == "true" || l_value == "1";
  }
  return in_default;
}

/// 会话产物目录。挂在资产根（kitsu_ctx_t::root_）下、按会话 uuid 命名 + 两级分桶。
///
/// 为什么不在 project.path_ 下：会话产物是 uuid 标识的生成物，与 sd2 / pictures / jobs
/// 同性质，本来就该和它们放在同一个根下。这样目录只依赖 root_ 与会话 uuid，
/// **不依赖 episode / shot / 项目路径** —— 于是没有「镜头没建资产扩展就传不了参考图」
/// 这一整类失败。
FSys::path session_dir(const ai_light_session& in_session) {
  return g_ctx().get<kitsu_ctx_t>().get_ai_light_session_dir(in_session.uuid_id_);
}

/// 删会话时把产物整体移进备份区。
///
/// 为什么不直接删：一次会话的产物（参考图 + 每轮 6 张渲染）动辄上百 MB，
/// 而且是「这版为什么没被选」的唯一证据。
///
/// 备份目标是 root_/ai_light/backup/<YYYY_MM_DD_HH>/<会话 uuid>/，与 up_file_base 的做法同构
/// （它备份到 root_path_/backup/<小时>/）：备份挂在**源自己这一片**根下，源与目标必然同卷，
/// rename 才不会因为「root_ 在本地盘、项目在 NAS」而失败。
void backup_session_dir(const ai_light_session& in_session) {
  auto l_src = session_dir(in_session);
  if (!FSys::exists(l_src)) return;
  try {
    auto l_dst = g_ctx().get<kitsu_ctx_t>().get_ai_light_backup_dir(
        in_session.uuid_id_, fmt::format("{:%Y_%m_%d_%H}", chrono::floor<chrono::hours>(chrono::system_clock::now()))
    );
    if (!FSys::exists(l_dst.parent_path())) FSys::create_directories(l_dst.parent_path());
    FSys::rename(l_src, l_dst);
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 产物已备份到 {}", in_session.uuid_id_, l_dst.generic_string()
    );
  } catch (const std::exception& in_err) {
    // 备份失败不能挡住删会话：留下一个没人引用的目录，比让用户删不掉会话要好。
    SPDLOG_LOGGER_ERROR(
        g_logger_ctrl().get_main_error(), "AI 灯光会话 {} 产物备份失败（会话仍会被删除）: {}", in_session.uuid_id_,
        in_err.what()
    );
  }
}

/// 定稿：把候选的 PNG 落成 preview_file 的**对象**（不落库，由 apply_judgement 同事务插入）。
///
/// 只有**定稿**才进 preview_file（中间候选走 /candidate/{id}/image 专用接口）——
/// 这是既定决策：中间候选是搜索过程的中间态，每轮 6 张全注册会把任务的预览列表冲垮，
/// 也会把「交付物」这个概念稀释掉。EXR 一律不注册、不下发浏览器。
preview_file build_accepted_preview_file(
    sqlite_database& in_sql, const ai_light_session& in_session, const ai_light_candidate& in_candidate,
    const uuid& in_person_id
) {
  // 落点是现算的（D44），所以这里只有一条判定：算出来的那张图在不在。
  const FSys::path l_png_path = candidate_render_path(g_ctx().get<kitsu_ctx_t>(), in_candidate, false);
  DOODLE_CHICK_HTTP(
      FSys::exists(l_png_path), bad_request, "候选 {} 的渲染图不存在（{}），无法定稿", in_candidate.uuid_id_,
      l_png_path.generic_string()
  );

  auto l_image = cv::imread(l_png_path.generic_string());
  DOODLE_CHICK_HTTP(!l_image.empty(), internal_server_error, "候选 {} 的渲染图无法读取", in_candidate.uuid_id_);

  preview_file l_pf{};
  // uuid 必须在这里就定下来，不能等 ORM 在 insert 时回填：
  // save_variants() 用 uuid 当文件名，nil uuid 会让所有定稿图写进同一个路径。
  l_pf.uuid_id_       = core_set::get_set().get_uuid();
  l_pf.task_id_       = in_session.task_id_;
  l_pf.person_id_     = in_person_id;
  l_pf.name_          = core_set::get_set().get_uuid_str();
  l_pf.original_name_ = fmt::format("ai_light_{}_cand{:02}", in_session.uuid_id_, in_candidate.candidate_index_);
  l_pf.extension_     = "png";
  l_pf.source_        = preview_file_source_enum::auto_light_generate;
  l_pf.status_        = preview_file_statuses::processing;
  l_pf.revision_      = in_sql.get_next_preview_revision(l_pf.task_id_);
  l_pf.position_      = in_sql.get_next_position(l_pf.task_id_, l_pf.revision_);

  // 变体落盘必须走 preview::save_variants —— 预览列表页读的就是它生成的那套尺寸，
  // 另写一套必然与既有预览文件不一致。
  auto l_size         = preview::save_variants(l_image, l_pf.uuid_id_);
  l_pf.width_         = l_size.width;
  l_pf.height_        = l_size.height;
  l_pf.path_          = g_ctx().get<kitsu_ctx_t>().get_pictures_preview_file(l_pf.uuid_id_);
  // path_ 不承载取图（前端按 uuid 走 /api/pictures/preview-files/{id}.png），
  // 这里只把大小记准，省得预览列表页显示 0 字节。
  l_pf.file_size_     = FSys::exists(l_pf.path_) ? FSys::file_size(l_pf.path_) : 0;
  l_pf.status_        = preview_file_statuses::ready;
  return l_pf;
}

/// 读会话详情并做权限校验。多处复用。
ai_light_session_detail_t load_detail_checked(
    const http_jwt_fun::http_jwt_t& in_person, sqlite_database& in_sql, const uuid& in_session_id
) {
  auto l_detail = build_session_detail(in_sql, in_session_id);
  if (!l_detail.has_value())
    throw_exception(
        http_request_error{boost::beast::http::status::not_found, fmt::format("AI 灯光会话 {} 不存在", in_session_id)}
    );
  in_person.check_project_access(l_detail->session_.project_id_);
  return std::move(*l_detail);
}

}  // namespace

// ===========================================================================
// 会话
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(shot_session_route, post) {
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_json   = in_handle->get_json();

  // 镜头是唯一的归属锚点：先查实体，**项目从实体上取**，不接受请求体传。
  //
  // 这不只是省一个入参。以前 project_id 由请求体给，服务端只能校验
  // 「用户对那个项目有没有权限」，**管不了镜头和项目是否真的一致** ——
  // 于是可以把 A 项目的镜头挂到 B 项目下（用户对 B 有权限即可），
  // 造出一条「镜头在 A、项目在 B」的会话，之后所有按项目的列表都会显示它。
  // 现在归属只有一个真相：entity.project_id_。
  //
  // 镜头不存在时 get_by_uuid → to_single 抛 sqlite_orm_exception{404}，由 HTTP 层统一翻。
  auto l_entity = l_sql.get_by_uuid<entity>(entity_id_);

  ai_light_session l_session{};
  l_json.get_to(l_session);
  l_session.submitter_  = person_.person_.uuid_id_;
  // 路径、实体、镜头的灯光任务分别覆盖请求体里的同名字段：
  // 归属只有一个真相，客户端传什么都改变不了它。
  l_session.entity_id_  = entity_id_;
  l_session.project_id_ = l_entity.project_id_;
  l_session.task_id_    = resolve_lighting_task_id(l_sql, entity_id_);
  person_.check_project_access(l_session.project_id_);

  DOODLE_CHICK_HTTP(
      !l_session.prompt_text_.empty(), bad_request, "缺少 prompt_text —— 提示词是权威意图，不允许空着建会话"
  );

  // 状态与轮次由服务端定，不接受客户端传（否则能伪造出 awaiting_judgement 的会话）
  l_session.state_             = ai_light_session_state::draft;
  l_session.current_iteration_ = 0;
  l_session.judgement_due_at_  = std::nullopt;
  l_session.failure_reason_    = "";
  // 标题默认取**镜头名**：会话锚在镜头上，镜头名比任务名更贴切，也少一次 task 查询。
  if (l_session.title_.empty()) l_session.title_ = l_entity.name_;

  {
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(insert(l_sql).into<ai_light_session>().values(l_session));
    co_await l_sql.run_sql(std::move(l_sqls));
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 为镜头 {} 创建 AI 灯光会话 {}（{} 字提示词）", person_.person_.email_,
      person_.person_.get_full_name(), l_session.entity_id_, l_session.uuid_id_, l_session.prompt_text_.size()
  );

  // ---------------------------------------------------------------
  // 建会话**不**蒸馏 Spec
  // ---------------------------------------------------------------
  // Spec 的输入是「提示词 + 参考图」，而参考图只能在会话存在**之后**上传
  // （reference_route 要 session_id，而 session_id 是本接口的返回值）。
  // 建会话时 get_ai_light_references() 必然为空，若在此处推进状态就会基于一份
  // 「只用提示词」的方案干活，参考图这个主输入被静默丢掉。
  //
  // 正确顺序：建会话（draft）→ 传参考图（推进到 generating）→ 提交候选。

  // 返回**多表聚合详情**而不是会话本体：形状与 GET /session/{id}、/judge 一致，
  // 前端只需要一套解析。新建会话时 references 与 candidates 都是空的。
  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(shot_session_route, get) {
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();

  // 镜头 id 来自路径，必填：本接口的定位是镜头页，归属明确，不提供
  // 「按 project_id 列出该项目全部会话」那种项目管理视角的查询 ——
  // 两种语义挤在一个 URL 上，权限检查的粒度也会跟着含糊
  // （按项目查只需查一次权限，按镜头查得逐个会话查）。
  auto l_sessions = l_sql.get_ai_light_sessions_by_entity(entity_id_);

  // 权限逐个会话校验：会话行的 project_id_ 是唯一权威来源。
  // 列表为空时不校验也不会漏任何东西（没有可泄露的内容）。
  for (const auto& l_s : l_sessions) person_.check_project_access(l_s.project_id_);

  co_return in_handle->make_msg(nlohmann::json{{"data", l_sessions}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_return in_handle->make_msg(nlohmann::json{} = load_detail_checked(person_, l_sql, id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, put) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  // id 来自路径；请求体里带了也忽略（路径为准，见 ai_light.h 顶部的约定）
  auto l_session = load_session_checked(person_, l_sql, id_);

  // 逐字段可选：给了才改。整行 PUT 会要求前端先把会话详情读回来再原样发一遍，
  // 而并发场景下（会话里最多只有一个人在改）那一次往返只会多出一次被静默回滚的机会。
  if (l_json.contains("prompt_text")) l_json.at("prompt_text").get_to(l_session.prompt_text_);

  // 提示词是**权威意图**（§6.3：提示词优先于参考图），空提示词会让 Spec 蒸馏失去依据 ——
  // 所以这里挡在写库之前，而不是留给 /spec POST 去报「没有参考图/无法蒸馏」。
  DOODLE_CHICK_HTTP(!l_session.prompt_text_.empty(), bad_request, "提示词不能为空");

  {
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(update(l_sql)
                            .from<ai_light_session>()
                            .set(c(&ai_light_session::prompt_text_) = l_session.prompt_text_)
                            .where(c(&ai_light_session::uuid_id_) == id_));
    co_await l_sql.run_sql(std::move(l_sqls));
  }

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "用户 {}({}) 修改了 AI 灯光会话 {} 的提示词（{} 字符）", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_session.prompt_text_.size()
  );

  // 返回整份会话详情，与 GET 同形：前端改完提示词往往要顺手刷新闸门 1 的显示。
  co_return in_handle->make_msg(nlohmann::json{} = load_detail_checked(person_, l_sql, id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, id_);

  // 会话没有终态，所以删会话不设前置闸门（§15.10 决定 D21）。
  //
  // 定稿图**不会**跟着会话一起消失：它由 preview::save_variants() 落在图片预览文件目录
  // （get_pictures_preview_file(uuid)），不在会话目录里；而 ai_light_candidate.preview_file_id_
  // → preview_file 的外键是 set_null（不是 cascade），所以删会话级联删候选之后，
  // preview_file 行仍然挂在灯光任务上，交付记录不会失去出处。
  // 下面备份的是会话自己的产物目录（参考图 + 每轮渲染的 PNG / EXR）。
  backup_session_dir(l_session);

  sql_modify_statement_vector_t l_sqls{};
  // 参考图 / Spec / 轮次 / 候选的外键都是 cascade，删会话行即连带删净。
  l_sqls.emplace_back(delete_from(l_sql).from<ai_light_session>().where(c(&ai_light_session::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 AI 灯光会话 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  co_return in_handle->make_msg_204();
}

// ===========================================================================
// 参考图
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_route, get) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, session_id_);
  co_return in_handle->make_msg(nlohmann::json{{"data", l_sql.get_ai_light_references(l_session.uuid_id_)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, session_id_);

  auto l_files   = in_handle->get_files();
  DOODLE_CHICK_HTTP(!l_files.empty(), bad_request, "请上传至少一张参考图");

  // 参考图目录在**会话目录之内**：<root>/ai_light/<分桶>/<会话 uuid>/ref/。
  // 不能用 session_dir().parent_path() / "ref" —— 那是会话目录的**兄弟**目录，
  // 会让同一镜头的所有会话共用一个 ref/，于是两个会话的参考图互相覆盖、
  // 删会话时又因为真正的会话目录不存在而备份不到。
  auto& l_ctx    = g_ctx().get<kitsu_ctx_t>();
  auto l_ref_dir = l_ctx.get_ai_light_reference_dir(l_session.uuid_id_);
  if (!FSys::exists(l_ref_dir)) FSys::create_directories(l_ref_dir);

  // 参考图不再有 ordinal 列: 展示顺序由 created_at 决定 (见 get_ai_light_references)。
  // 文件名是各自的 uuid, 所以任何顺序变化都不会覆盖既有图。

  std::vector<ai_light_reference> l_added{};
  for (auto&& l_file : l_files) {
    // 统一转 PNG：参考图来自「原始渲染 + AI/PS 修图」，格式五花八门，
    // 而下游统计与前端展示都只认 PNG。
    auto l_info = preview::convert_to_png(l_file);
    DOODLE_CHICK_HTTP(
        !l_info.path_.empty(), bad_request, "参考图 {} 无法解析为图片", l_file.filename().generic_string()
    );

    ai_light_reference l_ref{};
    l_ref.session_id_ = l_session.uuid_id_;
    // uuid 必须在这里就定下来，不能等 ORM 在 insert 时回填：文件名要用它，
    // nil uuid 会让所有参考图写进同一个路径（先例：build_accepted_preview_file）。
    l_ref.uuid_id_    = core_set::get_set().get_uuid();
    l_ref.file_path_  = l_ctx.get_ai_light_reference_file(l_session.uuid_id_, l_ref.uuid_id_);
    FSys::rename(l_info.path_, l_ref.file_path_);
    l_ref.width_    = l_info.size_.width;
    l_ref.height_   = l_info.size_.height;

    // 统计在**已落盘**的 PNG 上算：统计描述的是最终存下来的那个工件，
    // 不是转换过程中的中间产物（也就不会出现「库里的统计和文件里的像素不是同一张图」）。
    auto l_analysis = ai_light::analyze_reference_file(l_ref.file_path_);
    // 刚才在同一个请求里成功解码并转存过它，现在读不出属于内部不一致，不是可降级的情况 ——
    // 所以是 400 而不是「留空 + confidence = 0」。后者会让一条 0×0 的坏记录
    // 看起来像一张合法的、只是「统计不可用」的参考图。
    DOODLE_CHICK_HTTP(
        l_analysis.has_value(), bad_request, "参考图 {} 落盘后无法解码，无法统计",
        l_ref.file_path_.filename().generic_string()
    );
    // 用 set_stats 而不是直接赋值：nlohmann 的花括号初始化会静默变成单元素数组
    // （见 ai_light/common.h 顶部那个坑），而 set_stats 内部的赋值走 ADL 的 to_json。
    l_ref.set_stats(l_analysis->stats_);
    l_ref.stats_confidence_ = l_analysis->confidence_;
    // 外点判定需要跨图比较（§5.3），属于多图聚合，这里一律先标 false。
    l_ref.is_outlier_       = false;
    l_added.push_back(l_ref);
  }

  sql_modify_statement_vector_t l_sqls{};
  // 非 const：ORM 的 values() 会回填 uuid_id_ / created_at_（服务端生成字段）
  for (auto& l_ref : l_added) l_sqls.emplace_back(insert(l_sql).into<ai_light_reference>().values(l_ref));

  // ---- 参考图落库与「开始干活」同事务：draft → generating ----
  //
  // **上传参考图是唯一能观测到「输入齐了」的地方**：建会话时参考图还不可能存在
  // （reference_route 要 session_id，而 session_id 是建会话的返回值，见
  // shot_session_route::post 的说明），所以「会话存在 + 有参考图」这个组合
  // 第一次成立就在本函数返回之前的这一刻。
  //
  // 只推 draft：awaiting_judgement 时上传参考图**不得**改状态（那一轮的候选还在等判断），
  // generating 时也不必重复写（补图不改变「可以再出一轮」的语义）。
  if (l_session.state_ == ai_light_session_state::draft) {
    l_sqls.emplace_back(update(l_sql)
                            .from<ai_light_session>()
                            .set(c(&ai_light_session::state_) = ai_light_session_state::generating)
                            .where(c(&ai_light_session::uuid_id_) == l_session.uuid_id_));
  }
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 为 AI 灯光会话 {} 上传了 {} 张参考图", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_added.size()
  );
  co_return in_handle->make_msg(nlohmann::json{{"data", l_added}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();

  // 参考图不存在时 get_by_uuid 抛 sqlite_orm_exception{404}，由 http_session_data 统一翻成 404
  auto l_ref = l_sql.get_by_uuid<ai_light_reference>(id_);
  // 归属校验走会话：参考图自己没有项目字段，它的项目就是它所属会话的项目。
  load_session_checked(person_, l_sql, l_ref.session_id_);

  co_return in_handle->make_msg(nlohmann::json{{"data", l_ref}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, put) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  // 参考图不存在时 get_by_uuid 抛 sqlite_orm_exception{404}
  auto l_ref  = l_sql.get_by_uuid<ai_light_reference>(id_);
  // 权限校验的对象是会话（与 GET/DELETE 同一口径）：参考图行是唯一权威的归属来源，
  // 不让前端传 session_id 来「帮忙」—— 那会引入一处可伪造的输入。
  load_session_checked(person_, l_sql, l_ref.session_id_);

  // 逐字段可选：「给了才改」而不是整行覆盖。整行 PUT 会让前端必须先把当前值读回来
  // 再原样发一遍，中间任何一次并发改动都会被静默回滚 —— 而这里要改的只是一个标记。
  if (l_json.contains("is_outlier")) l_json.at("is_outlier").get_to(l_ref.is_outlier_);
  if (l_json.contains("excluded_reason")) l_json.at("excluded_reason").get_to(l_ref.excluded_reason_);

  // 排除原因只在「已排除」时有意义。取消排除时必须一并清掉，否则会留下
  // 「不在排除集里、却带着一条排除原因」的行 —— 重建 Spec 时读到这条记录的人
  // 无法判断它到底算不算被排除。
  if (!l_ref.is_outlier_) l_ref.excluded_reason_.clear();

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_reference>()
                          .set(
                              c(&ai_light_reference::is_outlier_)      = l_ref.is_outlier_,
                              c(&ai_light_reference::excluded_reason_) = l_ref.excluded_reason_
                          )
                          .where(c(&ai_light_reference::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "用户 {}({}) 把 AI 灯光参考图 {} 的排除标记改为 {}（原因：{}）",
      person_.person_.email_, person_.person_.get_full_name(), id_, l_ref.is_outlier_,
      l_ref.excluded_reason_.empty() ? "无" : l_ref.excluded_reason_
  );
  co_return in_handle->make_msg(nlohmann::json{{"data", l_ref}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();

  // 参考图不存在时 get_by_uuid 抛 sqlite_orm_exception{404}，由 http_session_data 统一翻成 404
  auto l_ref     = l_sql.get_by_uuid<ai_light_reference>(id_);
  auto l_session = load_session_checked(person_, l_sql, l_ref.session_id_);

  // 先删库再删文件：反过来的话，删文件成功而删库失败就会留下指向空文件的记录，
  // 而记录还在就仍会被拿去统计。
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<ai_light_reference>().where(c(&ai_light_reference::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));

  if (!l_ref.file_path_.empty()) {
    std::error_code l_ec{};
    FSys::remove(l_ref.file_path_, l_ec);
    if (l_ec)
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(), "参考图 {} 的文件 {} 删除失败: {}", id_, l_ref.file_path_.generic_string(),
          l_ec.message()
      );
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 AI 灯光参考图 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  co_return in_handle->make_msg_204();
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_image_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();

  // 参考图不存在时 get_by_uuid 抛 sqlite_orm_exception{404}，由 http_session_data 统一翻成 404
  auto l_ref = l_sql.get_by_uuid<ai_light_reference>(id_);
  person_.check_project_access(l_sql.get_by_uuid<ai_light_session>(l_ref.session_id_).project_id_);

  DOODLE_CHICK(FSys::exists(l_ref.file_path_), "参考图文件不存在 文件 {}", l_ref.file_path_.generic_string());
  co_return in_handle->make_msg(l_ref.file_path_, kitsu::mime_type(l_ref.file_path_.extension()));
}

// ===========================================================================
// 候选渲染图（中间候选专用接口；只有定稿才进 preview_file）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(candidate_image_route, get) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();

  // 候选不存在时 get_by_uuid 抛 sqlite_orm_exception{404}，由 http_session_data 统一翻成 404
  auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(id_);
  person_.check_project_access(l_sql.get_by_uuid<ai_light_session>(l_cand.session_id_).project_id_);

  // 未渲染与「文件丢了」是两件事，给不同的话术，省得美术以为是自己的操作错了。
  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::pending, not_found, "候选 {} 尚未渲染完成", l_cand.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::failed, not_found, "候选 {} 渲染失败", l_cand.uuid_id_
  );
  // 落点现算（D44）：候选行自带三个 uuid，不需要任何回查。
  const FSys::path l_png_path = candidate_render_path(g_ctx().get<kitsu_ctx_t>(), l_cand, false);
  DOODLE_CHICK(FSys::exists(l_png_path), "候选渲染图不存在 文件 {}", l_png_path.generic_string());

  // 只发 PNG（显示空间）。EXR 是线性数据、体积大且浏览器渲染不了，绝不下发。
  co_return in_handle->make_msg(l_png_path, kitsu::mime_type(l_png_path.extension()));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(candidate_movie_route, get) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();

  // 与取海报同一条路径：鉴权、存在性、状态判定都一样，只是产物换成视频。
  auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(id_);
  person_.check_project_access(l_sql.get_by_uuid<ai_light_session>(l_cand.session_id_).project_id_);

  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::pending, not_found, "候选 {} 尚未渲染完成", l_cand.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::failed, not_found, "候选 {} 渲染失败", l_cand.uuid_id_
  );
  // 落点现算（D44）：候选行自带三个 uuid，不需要任何回查。
  const FSys::path l_movie_path = candidate_movie_path(g_ctx().get<kitsu_ctx_t>(), l_cand);
  DOODLE_CHICK(FSys::exists(l_movie_path), "候选视频不存在 文件 {}", l_movie_path.generic_string());

  // 视频本体直发：make_msg(path, ...) 带 Range 支持，浏览器可以拖动进度条。
  co_return in_handle->make_msg(l_movie_path, kitsu::mime_type(l_movie_path.extension()));
}

// ===========================================================================
// 人工闸门 2：判断
// ===========================================================================
//
// 状态机只有这一个闸门。状态由提交接口自己推进：
//   建会话 → draft
//   上传参考图（reference_route，POST 里推进到 generating —— 见该处注释）
//   前两轮提交候选 → POST /session/{}/iteration（要求 generating）
//   渲染回传 → awaiting_judgement（mark_iteration_rendered）
//   判断 → generating（apply_judgement）

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(judge_route, post) {
  person_.check_not_outsourcer();
  auto l_sql       = get_sqlite_database();
  auto l_json      = in_handle->get_json();

  // session_id 来自路径；请求体里带了也忽略（路径为准）
  auto l_session   = load_session_checked(person_, l_sql, session_id_);

  auto l_iteration = l_sql.get_ai_light_iteration(l_session.uuid_id_, l_session.current_iteration_);
  DOODLE_CHICK_HTTP(l_iteration.has_value(), bad_request, "会话 {} 还没有可判断的轮次", l_session.uuid_id_);

  ai_light_judgement_t l_judgement{};
  l_json.get_to(l_judgement);
  l_judgement.by_ = person_.person_.uuid_id_;
  l_judgement.at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

  std::optional<preview_file> l_preview_file{};
  if (!l_judgement.selected_candidate_id_.is_nil()) {
    auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(l_judgement.selected_candidate_id_);
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == l_iteration->uuid_id_, bad_request, "候选 {} 不属于第 {} 轮", l_cand.uuid_id_,
        l_iteration->iteration_
    );
    l_preview_file = build_accepted_preview_file(l_sql, l_session, l_cand, person_.person_.uuid_id_);
  }

  co_await apply_judgement(l_sql, l_session, *l_iteration, l_judgement, l_preview_file);

  if (l_preview_file.has_value()) {
    socket_io::broadcast(
        socket_io::preview_file_new_broadcast_t{
            .preview_file_id_ = l_preview_file->uuid_id_,
            .comment_id_      = uuid{},
            .project_id_      = l_session.project_id_,
        }
    );
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 判断 AI 灯光会话 {} 第 {} 轮: {}{}", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_iteration->iteration_,
      l_preview_file.has_value() ? "定稿" : (l_judgement.selected_candidate_id_.is_nil() ? "全否" : "续轮"),
      l_preview_file.has_value() ? "（已定稿并注册预览文件）" : ""
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
}

// ===========================================================================
// 工作机回传：一轮候选的渲染结果（方案 §15.12）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(render_report_route, post) {
  // 调用方是 doodle worker（不是浏览器）：鉴权沿用 data_jobs_instance 那套 JWT ——
  // 工作机的 kitsu_client 用同一组凭据回传任务状态，所以这里也走 person_；
  // 但**不**做 check_project_access：工作机不是项目成员，它只认任务 uuid。
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  // task_id 来自路径，回传体里不带 session/iteration：轮次由服务端按任务 uuid 反查
  // （get_ai_light_iteration_by_task），这是两边唯一认的关联方式。
  auto l_task = l_sql.get_by_uuid<server_task_info>(task_id_);
  DOODLE_CHICK_HTTP(
      l_task.type_ == server_task_info_type::ai_light, bad_request, "任务 {} 的类型是 {}，不是 AI 灯光任务", task_id_,
      magic_enum::enum_name(l_task.type_)
  );

  // 形状错误在这里抛 502；语义问题交给 apply_render_report 裁决：
  // 整轮失败 → 只写会话的 failure_reason_（不是终态）；至少一张渲染成功 → 落库并转 awaiting_judgement。
  // ctx 传进去只为一件事：算产物落点（D41 的存在性判定要现算）。
  // locator 传空：队列路线的轮次由任务 uuid 定位（D54）。
  auto l_report = parse_round_render_report(l_json);
  co_await apply_render_report(l_sql, g_ctx().get<kitsu_ctx_t>(), task_id_, ai_light_iteration_locator_t{}, l_report);

  // 回执带上轮次与两条状态，工作机据此写日志。人工判断闸门不在这里。
  nlohmann::json l_out{{"task_id", task_id_}};
  if (auto l_it = l_sql.get_ai_light_iteration_by_task(task_id_); l_it.has_value()) {
    auto l_session           = l_sql.get_by_uuid<ai_light_session>(l_it->session_id_);
    l_out["session_id"]      = l_it->session_id_;
    l_out["iteration"]       = l_it->iteration_;
    l_out["iteration_state"] = l_it->state_;
    l_out["session_state"]   = l_session.state_;
    l_out["failure_reason"]  = l_session.failure_reason_;
  }
  co_return in_handle->make_msg(l_out);
}

// ===========================================================================
// 引擎内路线回传：一轮候选的渲染结果（D54）
// ===========================================================================
//
// 与上面那条工作机回传的差别只有两处，其余全部复用：
//   1) 定位方式：轮次在路径上（session_id + iteration），由 ai_light_iteration_locator_t
//      交给 apply_render_report。引擎内路线**没有 server_task_info 行**（D53：面板不派发），
//      所以拿不出任务 uuid，这也是 locator 存在的唯一理由。
//   2) 权限：调用方是本机 Doodle 客户端，用发起人自己的 JWT，所以做 check_project_access。
//      工作机那条相反（它不是项目成员，只认任务 uuid）；这里不放开。
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(render_report_local_route, post) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  // 会话必须存在且调用者有权限 —— 顺带把 project 归属查出来，省一次往返。
  auto l_session = load_session_checked(person_, l_sql, session_id_);

  // 轮次存在性在这里先挡一道：locator 找不到轮次时 apply_render_report 会静默返回 nullopt
  // （那是队列路线「任务还在排队」的正常状态），但在这条路线上找不到轮次一定是调用方搞错了，
  // 静默吞掉会变成「回传成功、状态没动」这种最难查的失败。
  DOODLE_CHICK_HTTP(
      l_sql.get_ai_light_iteration(l_session.uuid_id_, iteration_).has_value(), not_found, "会话 {} 没有第 {} 轮",
      l_session.uuid_id_, iteration_
  );

  // 形状错误抛 502；语义问题（D41 产物缺失降级、D42 EXR 清理、全灭写 failure_reason_）
  // 全部由 apply_render_report 裁决，与工作机路线共用同一段代码。
  auto l_report = parse_round_render_report(l_json);
  // 任务号给 nil：引擎内路线没有 server_task_info 行，定位完全靠 locator（D54）。
  co_await apply_render_report(
      l_sql, g_ctx().get<kitsu_ctx_t>(), uuid{}, ai_light_iteration_locator_t{l_session.uuid_id_, iteration_}, l_report
  );

  // 回执与工作机那条保持同形（task_id 除外 —— 这条路线没有任务），
  // 让两边共用同一份解析代码的本机客户端不必分叉。
  nlohmann::json l_out{};
  l_out["session_id"] = l_session.uuid_id_;
  l_out["iteration"]  = iteration_;
  std::string l_it_state{};
  std::string l_session_state{};
  if (auto l_it = l_sql.get_ai_light_iteration(l_session.uuid_id_, iteration_); l_it.has_value()) {
    auto l_session_after     = l_sql.get_by_uuid<ai_light_session>(l_it->session_id_);
    l_out["iteration_state"] = l_it->state_;
    l_out["session_state"]   = l_session_after.state_;
    l_out["failure_reason"]  = l_session_after.failure_reason_;
    // 日志单独取名字而不是从 json 里 value() 回来：json 里的枚举是字符串（magic_enum 名），
    // 从 json 取会得到 json 对象，fmt 不认，得先 get<std::string>()。
    l_it_state               = std::string{magic_enum::enum_name(l_it->state_)};
    l_session_state          = std::string{magic_enum::enum_name(l_session_after.state_)};
  }

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光（引擎内路线）：会话 {} 第 {} 轮回传已受理，状态 {} / {}", l_session.uuid_id_,
      iteration_, l_it_state, l_session_state
  );
  co_return in_handle->make_msg(l_out);
}

// ===========================================================================
// 工作机上传候选产物（首帧海报 / 候选视频 / 线性 EXR）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(candidate_file_route, post) {
  // 与 render_report_route 同类：调用方是工作机，不做项目权限检查（它不是项目成员）。
  person_.check_not_outsourcer();

  // 请求体必须是文件本身。任何 application/* 都会被框架归到 application_nuknown
  // （http_content_type.cpp:17），kitsu_client::upload_asset_file 发的正是 octet-stream；
  // 显式挡一道是为了让「误把 JSON 发到这个路由」变成一句明确的 400，
  // 而不是落到下面 get_file() 返回空路径再报「上传体为空」。
  DOODLE_CHICK_HTTP(
      in_handle->content_type_ == detail::content_type::application_nuknown, bad_request,
      "候选产物的上传体必须是 application/octet-stream，收到的是 {}", magic_enum::enum_name(in_handle->content_type_)
  );

  // 这里刻意**不**复用 up_file_base 那套（root_path_ / gen_file_path / move_file）：
  // 那套是「按项目+任务类型派生目录」，它的输入是项目与任务；本路由的落点由**候选行**决定
  // （candidate_render_path）。硬套过来要么多一层无意义的目录推导，要么得把
  // move_file 的 rename 换成跨卷安全的复制（http 缓存根与服务端数据根常常不同盘），
  // 两件都是在给一条直路加弯。
  auto l_sql  = get_sqlite_database();
  // 候选不存在时 get_by_uuid 抛 sqlite_orm_exception{404}，由 http_session_data 统一翻成 404。
  auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(id_);

  // 只挡 judged：轮次判定之后就不该再有字节进来（迟到的 worker 会改掉美术已经看过的图）。
  // awaiting_judgement 期间**允许**重传 —— 「上传失败后重试」是正常路径，不该被状态机挡住。
  auto l_it   = l_sql.get_by_uuid<ai_light_iteration>(l_cand.iteration_id_);
  DOODLE_CHICK_HTTP(
      l_it.state_ != ai_light_iteration_state::judged, bad_request, "AI 灯光轮次 {} 已判定，不再接受候选产物上传",
      l_it.iteration_
  );

  // 落点现算（D27/D40）：请求里根本没有路径信息，工作机伪造不了落点，
  // 最多只能往「服务端按命名规则算出来的那个文件名」里灌字节。
  // 与取海报/取视频路由、定稿路由、D41 的存在性判定用的是同一套函数。
  auto& l_ctx         = g_ctx().get<kitsu_ctx_t>();
  const bool l_linear = role_ == doodle::ai_light::ai_light_candidate_file_role::render_linear;
  const bool l_movie  = role_ == doodle::ai_light::ai_light_candidate_file_role::render_movie;
  const FSys::path l_dst =
      l_movie ? candidate_movie_path(l_ctx, l_cand) : candidate_render_path(l_ctx, l_cand, l_linear);

  // 请求体已经被框架流到临时文件（save_bode_file → cache_root("http")/<uuid><ext>），
  // 这里只是把它搬到落点。用 copy + remove 而不是 up_file_base 那套 rename：
  // 目标已存在时 Windows 的 rename 会直接失败，而重传同一个候选是允许的。
  const FSys::path l_tmp = in_handle->get_file();
  DOODLE_CHICK_HTTP(FSys::exists(l_tmp), bad_request, "候选 {} 的上传体为空", id_);

  std::error_code l_ec{};
  if (l_dst.has_parent_path()) {
    FSys::create_directories(l_dst.parent_path(), l_ec);
    DOODLE_CHICK_HTTP(
        !l_ec, internal_server_error, "候选 {} 产物目录创建失败 {}：{}", id_, l_dst.parent_path().generic_string(),
        l_ec.message()
    );
  }
  // 先写临时名再改名，避免「上传到一半」被前端读到残缺的产物：
  // 目标那个文件名正是取海报/取视频路由要读的，原地写会让过程中出现一个不完整的文件
  // （视频尤其明显 —— 半截 mp4 播放器直接报错，而且可能被浏览器缓存住）。
  const FSys::path l_tmp_dst = l_dst.string() + ".uploading";
  FSys::copy_file(l_tmp, l_tmp_dst, FSys::copy_options::overwrite_existing, l_ec);
  DOODLE_CHICK_HTTP(
      !l_ec, internal_server_error, "候选 {} 产物写入失败 {}：{}", id_, l_tmp_dst.generic_string(), l_ec.message()
  );
  FSys::rename(l_tmp_dst, l_dst, l_ec);
  DOODLE_CHICK_HTTP(
      !l_ec, internal_server_error, "候选 {} 产物落位失败 {}：{}", id_, l_dst.generic_string(), l_ec.message()
  );
  // 临时文件搬完就删（up_file_base 是 rename 走的，不留；这里是 copy，得自己收尾）。
  std::error_code l_ignored{};
  FSys::remove(l_tmp, l_ignored);

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光：候选 {}（轮次 {}）上传{}到位 {} {} 字节", id_, l_it.iteration_,
      l_movie ? "候选视频" : (l_linear ? "线性 EXR" : "首帧海报"), l_dst.generic_string(),
      FSys::file_size(l_dst, l_ignored)
  );
  co_return in_handle->make_msg_204();
}

// ===========================================================================
// 预设库
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_route, get) {
  // 预设库是全局共享资产，读权限放宽到「非外包」即可：
  // 美术要能看到彼此攒的预设才有意义。
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_only = query_bool(in_handle->url_, "enabled_only", true);
  co_return in_handle->make_msg(nlohmann::json{{"data", l_sql.get_ai_light_presets(l_only)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_route, post) {
  // 写权限同样是「非外包」：决策 13 说第一批预设由美术整理，
  // 所以不能要求 producer 级别；created_by_ 记录贡献者，异常内容事后可追。
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  ai_light_preset l_preset{};
  l_json.get_to(l_preset);
  DOODLE_CHICK_HTTP(!l_preset.name_.empty(), bad_request, "缺少预设名称");
  l_preset.created_by_ = person_.person_.uuid_id_;

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<ai_light_preset>().values(l_preset));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 新建 AI 灯光预设 {}（{}）", person_.person_.email_,
      person_.person_.get_full_name(), l_preset.uuid_id_, l_preset.name_
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_preset);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_preset = l_sql.get_ai_light_preset(id_);
  DOODLE_CHICK_HTTP(l_preset.has_value(), not_found, "预设 {} 不存在", id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_preset);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_instance_route, put) {
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_preset = l_sql.get_ai_light_preset(id_);
  DOODLE_CHICK_HTTP(l_preset.has_value(), not_found, "预设 {} 不存在", id_);

  // 局部更新：只覆盖请求体里出现的字段，避免前端漏传一个字段就把内容清空。
  auto l_json = in_handle->get_json();
  if (l_json.contains("name")) l_json.at("name").get_to(l_preset->name_);
  if (l_json.contains("tags")) l_json.at("tags").get_to(l_preset->tags_);
  if (l_json.contains("embedding")) l_json.at("embedding").get_to(l_preset->embedding_);
  if (l_json.contains("embedding_dim")) l_json.at("embedding_dim").get_to(l_preset->embedding_dim_);
  if (l_json.contains("rig_fragment")) l_json.at("rig_fragment").get_to(l_preset->rig_fragment_);
  if (l_json.contains("applicable")) l_json.at("applicable").get_to(l_preset->applicable_);
  if (l_json.contains("stats_target")) l_json.at("stats_target").get_to(l_preset->stats_target_);
  if (l_json.contains("enabled")) l_json.at("enabled").get_to(l_preset->enabled_);

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(update(l_sql)
                          .from<ai_light_preset>()
                          .set(
                              c(&ai_light_preset::name_)          = l_preset->name_,
                              c(&ai_light_preset::tags_)          = l_preset->tags_,
                              c(&ai_light_preset::embedding_)     = l_preset->embedding_,
                              c(&ai_light_preset::embedding_dim_) = l_preset->embedding_dim_,
                              c(&ai_light_preset::rig_fragment_)  = l_preset->rig_fragment_,
                              c(&ai_light_preset::applicable_)    = l_preset->applicable_,
                              c(&ai_light_preset::stats_target_)  = l_preset->stats_target_,
                              c(&ai_light_preset::enabled_)       = l_preset->enabled_
                          )
                          .where(c(&ai_light_preset::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));
  co_return in_handle->make_msg(nlohmann::json{} = *l_preset);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(preset_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql    = get_sqlite_database();
  auto l_preset = l_sql.get_ai_light_preset(id_);
  DOODLE_CHICK_HTTP(l_preset.has_value(), not_found, "预设 {} 不存在", id_);

  // 真删。预设没有外键引用（候选只把 rig 的副本存进 JSON），所以删掉不会破坏历史记录 ——
  // 这正是「候选存 rig 全量而不是引用预设 id」换来的好处。
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<ai_light_preset>().where(c(&ai_light_preset::uuid_id_) == id_));
  co_await l_sql.run_sql(std::move(l_sqls));
  co_return in_handle->make_msg_204();
}

// ---------------------------------------------------------------------------
// 推理网关（角色降级 §3.1）
// ---------------------------------------------------------------------------
//
// 只做三件事：权限 → 跑推理 → 组响应。
// **不组装提示词、不解释模型输出** —— 那两件事降级后都在前端。
//
// 请求体**原样透传**（见 build_gateway_inference_request）：除 `model`（服务端强制）外，
// 前端给的键一律原样发往方舟 `/responses`。所以新增字段、用上 tools / text.format /
// 多模态输入都不需要动这里。前端请求体的形状以火山方舟 Responses API 为准
// （input / instructions / text.format ...），不再是本系统自己那套扁平键
// （input_text / json_schema）：旧扁平键按契约**静默忽略**。
//
// 推理管道（凭据解析 → 任务信封 → 计费 → 成败判定）就在下面，不另立公共层：
//   * 任务行落库的就是**要发出去的那份** json，不经结构体往返 —— 那次往返是一份手写白名单，
//     会把前端透传进来的方舟键（input / tools / text.format…）静默吃掉。
//   * 不传 text_prompt_：任务行要显示的那句话由 collect_request_info() 落库时自己从请求体读；
//     调用方再抄一份进来就有了第二个真相，且抄的那份往往还是截断过的。
//   * 副作用**只有** run_inference_task 自己那个事务（任务行 + 扣费），**有意保留**：
//     推理确实发生了，必须留痕计费。
//
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(inference_route, post) {
  // 外包人员不能触发推理 —— 与 judge_route 同一口径。
  // 它花的是工作室的钱，不是「只看一眼」的只读操作。
  person_.check_not_outsourcer();

  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  // ---- 1) 权限：会话归属（路径里的 {session_id} → check_project_access）----
  //
  // 会话 id 走**路径**（/session/{session_id}/inference），与 /session/{session_id}/spec、
  // /iterate、/judge 完全同形 —— 于是「鉴权查的对象」与「URL 上操作的对象」必然同一个，
  // 不会出现「路径说 A、体里说 B，权限查了 A、计费记到 B」。
  // 也不从体里取 project_id：会话行自己就知道它属于哪个项目。
  //
  // 会话不存在时 get_by_uuid → to_single 抛 sqlite_orm_exception{404}，
  // 由 http_session_data 统一翻成 404（与 load_session_checked 同一口径）。
  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  // ---- 2) 计费元数据 ----
  //
  // `operator_id_` **只从 JWT 取**。请求体里若有 operator_id 键会被彻底忽略 ——
  // 这是结构性保证：没有任何代码路径能让外部值流进来（见 ai_light.h 的说明）。
  // session_id / project_id 只为任务归属可查，下面的解析用不到它们（网关不走
  // 落库信封那条路，见 build_gateway_inference_request）。
  const uuid& l_operator_id = person_.person_.uuid_id_;

  // ---- 3) 凭据：触发人 → person → ai_studio → 推理客户端 ----
  //
  // 用 select + to_optional 而不是 get_by_uuid<T>：后者内部是 to_single()，「查不到」会变成
  // 一条 sqlite 异常（HTTP 500），而这里应当是 400 语义。
  // 每次推理都**新建**客户端：http_client_ssl 内部只有一个 socket 且无锁。
  DOODLE_CHICK_HTTP(
      !l_operator_id.is_nil(), bad_request, "推理缺少触发人（operator_id），无法解析 AI 工作室与计费主体"
  );
  const auto l_person = select(l_sql)
                            .columns(object<person>())
                            .from<person>()
                            .where(c(&person::uuid_id_) == l_operator_id)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_person.has_value(), bad_request, "推理找不到触发人 {}（person 不存在），无法解析 AI 工作室", l_operator_id
  );
  DOODLE_CHICK_HTTP(
      !l_person->ai_studio_id_.is_nil(), bad_request, "推理的触发人 {} 未关联 AI 工作室，无法调用推理接口",
      l_operator_id
  );
  const auto l_studio = select(l_sql)
                            .columns(object<ai_studio>())
                            .from<ai_studio>()
                            .where(c(&ai_studio::uuid_id_) == l_person->ai_studio_id_)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_studio.has_value(), bad_request, "推理找不到 AI 工作室 {}（配置可能已被删除）", l_person->ai_studio_id_
  );

  // ---- 4) 组装任务信封 ----
  //
  // 全名写 `doodle::seedance2::task`：本函数所在命名空间同时 `using namespace doodle::ai_light`
  // 与 `using namespace orm`，而 `doodle::task` 这个名字也被占据 —— 写短名 `seedance2::task`
  // 会被解析成 `doodle::task`。
  doodle::seedance2::task l_task{};
  l_task.type_                = doodle::seedance2::task_type::inference;     // not_null，必须显式赋值
  l_task.backend_             = doodle::seedance2::task_backend::seedance2;  // 同上
  l_task.user_id_             = l_operator_id;                               // 计费主体 + 签名 URL 的 uid
  l_task.ai_studio_id_        = l_studio->uuid_id_;
  l_task.project_uuid_id_     = l_session.project_id_;  // 只为归属可查，内部不依赖
  // 归属会话：会话就在路径上（/session/{session_id}/inference），所以这次推理属于哪个会话
  // 在网关这里是**已知的**。轮次则不然 —— 降级后轮次由前端在 /session/{}/iteration 提交时
  // 建立，网关跑推理时它还不存在，所以任务行只指会话、不指轮次。
  l_task.ai_light_session_id_ = l_session.uuid_id_;
  l_task.completion_tokens_   = doodle_config::g_ark_inference_default_consumed_tokens;
  l_task.data_request_        = l_json;

  // ---- 5) 跑推理（凭据 + 计费 + 成败判定全在这一句里，不复制）----
  //
  // run_inference_task 自己一个事务里落终态任务行 + 扣费。这是唯一的副作用，且**有意保留**：
  // 推理确实发生了，必须留痕计费 —— 否则调用方回滚会话状态时会连带把 person_token 的扣费抹掉。
  auto l_result =
      co_await http::seedance2::run_inference_task(l_task, l_sql, http::seedance2::make_inference_client(*l_studio));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光推理网关：会话 {} 完成一次推理（{} → {} token）", l_session.uuid_id_,
      l_result.input_tokens_, l_result.output_tokens_
  );

  // ---- 6) 组响应。原始结果回传，服务端不做任何解释 ----
  //
  // **不带 task_id**：run_task_result_t 里没有这个字段，要带得改它的返回，
  // 不列入第一梯队（见 C++ 计划 §7 第 1 条）。计费留痕在 seedance2::task 行里。
  co_return in_handle->make_msg(
      nlohmann::json{
          {"result", l_result.result_json_},
          {"input_tokens", l_result.input_tokens_},
          {"output_tokens", l_result.output_tokens_},
          {"total_tokens", l_result.total_tokens_},
      }
  );
}

}  // namespace doodle::http::ai_light
