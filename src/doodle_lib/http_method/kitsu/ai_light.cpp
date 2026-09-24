//
// AI 灯光：HTTP 处理函数
//
// 每个处理函数只做四件事：权限 → 校验入参 → 调编排层 → 组响应。
// 状态推进与业务规则一律在 ai_light_orchestrator.cpp，这里不写状态机。
//
// 关于两个「本轮返回 501」的端点（/spec POST 与 /iterate）：
// 它们**不是**占位桩 —— 权限、状态校验、会话查找全部真实执行，只有最里层的
// LLM 接缝抛 not_implemented。所以现在调它们得到的是「参数错了会明确报错、
// 参数对了告诉你能力还没到」这种诚实的行为，而不是一个假装成功的 200。
//

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/task.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/http_method/kitsu/preview.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/beast/http/status.hpp>
#include <magic_enum/magic_enum.hpp>
#include <opencv2/opencv.hpp>

#include <algorithm>
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
ai_light_session load_session_checked(const http_jwt_fun::http_jwt_t& in_person, sqlite_database& in_sql, const uuid& in_session_id) {
  auto l_session = in_sql.get_by_uuid<ai_light_session>(in_session_id);
  in_person.check_project_access(l_session.project_id_);
  return l_session;
}

/// 终态会话一律只读
void check_session_mutable(const ai_light_session& in_session) {
  DOODLE_CHICK_HTTP(
      !is_terminal(in_session.state_), bad_request, "会话 {} 已处于终态 {}，不能再修改",
      in_session.uuid_id_, magic_enum::enum_name(in_session.state_)
  );
}

std::optional<uuid> query_uuid(const boost::url& in_url, std::string_view in_key) {
  for (auto&& [l_key, l_value, l_has] : in_url.params()) {
    if (l_key == in_key && l_has) return from_uuid_str(l_value);
  }
  return std::nullopt;
}

bool query_bool(const boost::url& in_url, std::string_view in_key, bool in_default) {
  for (auto&& [l_key, l_value, l_has] : in_url.params()) {
    if (l_key == in_key && l_has) return l_value == "true" || l_value == "1";
  }
  return in_default;
}

/// 会话产物目录的绝对路径。拿不到基准时返回 nullopt（例如镜头还没建资产扩展）。
std::optional<FSys::path> session_dir(sqlite_database& in_sql, const ai_light_session& in_session) {
  try {
    auto l_shot = in_sql.get_by_uuid<entity>(in_session.entity_id_);
    if (l_shot.parent_id_.is_nil()) return std::nullopt;
    auto l_prj           = in_sql.get_by_uuid<project>(in_session.project_id_);
    auto l_episode       = episodes{in_sql.get_by_uuid<entity>(l_shot.parent_id_)};
    auto l_ground_extend = in_sql.get_entity_shot_extend_by_task(in_session.entity_id_);
    return l_prj.path_ / get_ai_light_session_path(l_episode, l_ground_extend, in_session.uuid_id_);
  } catch (const std::exception&) {
    return std::nullopt;
  }
}

/// 删会话时把产物整体移进项目备份区。
///
/// 为什么不直接删：一次会话的产物（参考图 + 每轮 6 张渲染）动辄上百 MB，
/// 而且是「这版为什么没被选」的唯一证据。up_file_base 的既有做法就是先进
/// project.path_/backup/<YYYY_MM_DD_HH>/，这里沿用同一套语义。
void backup_session_dir(sqlite_database& in_sql, const ai_light_session& in_session) {
  auto l_src = session_dir(in_sql, in_session);
  if (!l_src.has_value() || !FSys::exists(*l_src)) return;
  try {
    auto l_prj = in_sql.get_by_uuid<project>(in_session.project_id_);
    auto l_dst = l_prj.path_ / "backup" /
                 fmt::format("{:%Y_%m_%d_%H}", chrono::floor<chrono::hours>(chrono::system_clock::now())) /
                 fmt::format("ai_light_{}", in_session.uuid_id_);
    if (!FSys::exists(l_dst.parent_path())) FSys::create_directories(l_dst.parent_path());
    FSys::rename(*l_src, l_dst);
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
  DOODLE_CHICK_HTTP(
      !in_candidate.render_png_path_.empty(), bad_request, "候选 {} 还没有渲染产物，无法定稿",
      in_candidate.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      FSys::exists(in_candidate.render_png_path_), bad_request,
      "候选 {} 的渲染图不存在（{}），无法定稿", in_candidate.uuid_id_, in_candidate.render_png_path_.generic_string()
  );

  auto l_image = cv::imread(in_candidate.render_png_path_.generic_string());
  DOODLE_CHICK_HTTP(
      !l_image.empty(), internal_server_error, "候选 {} 的渲染图无法读取",
      in_candidate.uuid_id_
  );

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
  auto l_size = preview::save_variants(l_image, l_pf.uuid_id_);
  l_pf.width_  = l_size.width;
  l_pf.height_ = l_size.height;
  l_pf.path_   = g_ctx().get<kitsu_ctx_t>().get_pictures_preview_file(l_pf.uuid_id_);
  // path_ 不承载取图（前端按 uuid 走 /api/pictures/preview-files/{id}.png），
  // 这里只把大小记准，省得预览列表页显示 0 字节。
  l_pf.file_size_ = FSys::exists(l_pf.path_) ? FSys::file_size(l_pf.path_) : 0;
  l_pf.status_    = preview_file_statuses::ready;
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

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  ai_light_session l_session{};
  l_json.get_to(l_session);
  l_session.submitter_ = person_.person_.uuid_id_;

  DOODLE_CHICK_HTTP(
      !l_session.project_id_.is_nil(), bad_request, "缺少 project_id"
  );
  DOODLE_CHICK_HTTP(!l_session.task_id_.is_nil(), bad_request, "缺少 task_id");
  DOODLE_CHICK_HTTP(!l_session.entity_id_.is_nil(), bad_request, "缺少 entity_id");
  DOODLE_CHICK_HTTP(
      !l_session.prompt_text_.empty(), bad_request,
      "缺少 prompt_text —— 提示词是权威意图，不允许空着建会话"
  );
  person_.check_project_access(l_session.project_id_);

  // 状态与轮次由服务端定，不接受客户端传（否则能伪造出 awaiting_judgement 的会话）
  l_session.state_             = ai_light_session_state::draft;
  l_session.current_iteration_ = 0;
  l_session.judgement_due_at_  = std::nullopt;
  l_session.suspended_at_      = std::nullopt;
  l_session.failure_reason_    = "";
  if (l_session.title_.empty())
    l_session.title_ = l_sql.get_by_uuid<task>(l_session.task_id_).name_;

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(insert(l_sql).into<ai_light_session>().values(l_session));
  co_await l_sql.run_sql(std::move(l_sqls));

  // 挂起扫描器懒启动：第一次有 AI 灯光请求时拉起，之后常驻。
  // 放在这里而不是启动流程里，是为了不依赖任何初始化顺序。
  ai_light_suspend_scanner::Get().run();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 创建 AI 灯光会话 {}（镜头 {}，{} 字提示词）", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_session.entity_id_, l_session.prompt_text_.size()
  );
  co_return in_handle->make_msg(nlohmann::json{} = l_session);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();

  auto l_entity_id  = query_uuid(in_handle->url_, "entity_id");
  auto l_project_id = query_uuid(in_handle->url_, "project_id");
  DOODLE_CHICK_HTTP(
      l_entity_id.has_value() || l_project_id.has_value(), bad_request,
      "需要 entity_id 或 project_id 之一"
  );

  std::vector<ai_light_session> l_sessions{};
  if (l_entity_id.has_value()) {
    // 镜头页入口：查该镜头下全部会话（含历史），新的在前
    l_sessions = l_sql.get_ai_light_sessions_by_entity(*l_entity_id);
    for (const auto& l_s : l_sessions) person_.check_project_access(l_s.project_id_);
  } else {
    person_.check_project_access(*l_project_id);
    l_sessions = l_sql.get_ai_light_sessions_by_project(*l_project_id);
  }

  co_return in_handle->make_msg(nlohmann::json{{"data", l_sessions}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_return in_handle->make_msg(nlohmann::json{} = load_detail_checked(person_, l_sql, id_));
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(session_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_session = load_session_checked(person_, l_sql, id_);

  // 定稿的会话禁止删除：它的 preview_file 已经挂在任务上，删掉会话会让交付记录失去出处。
  // 要撤销就先在别处显式撤销定稿（候选上有 clear_accepted() 与之配套）。
  DOODLE_CHICK_HTTP(
      l_session.state_ != ai_light_session_state::accepted, bad_request,
      "会话 {} 已定稿，不能删除；定稿候选已作为预览文件挂在任务上", l_session.uuid_id_
  );

  backup_session_dir(l_sql, l_session);

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
  check_session_mutable(l_session);

  auto l_files = in_handle->get_files();
  DOODLE_CHICK_HTTP(!l_files.empty(), bad_request, "请上传至少一张参考图");

  auto l_dir = session_dir(l_sql, l_session);
  DOODLE_CHICK_HTTP(
      l_dir.has_value(), bad_request,
      "镜头 {} 缺少资产扩展信息，无法推算参考图目录", l_session.entity_id_
  );
  auto l_ref_dir = l_dir->parent_path() / "ref";
  if (!FSys::exists(l_ref_dir)) FSys::create_directories(l_ref_dir);

  // 已有参考图占用的序号，新图从 max+1 开始 —— 不覆盖既有图。
  auto l_existing = l_sql.get_ai_light_references(l_session.uuid_id_);
  std::int32_t l_next = 0;
  for (const auto& l_r : l_existing) l_next = std::max(l_next, l_r.ordinal_ + 1);

  std::vector<ai_light_reference> l_added{};
  for (auto&& l_file : l_files) {
    // 统一转 PNG：参考图来自「原始渲染 + AI/PS 修图」，格式五花八门，
    // 而下游统计（未落地）与前端展示都只认 PNG。
    auto l_info = preview::convert_to_png(l_file);
    DOODLE_CHICK_HTTP(
        !l_info.path_.empty(), bad_request, "参考图 {} 无法解析为图片",
        l_file.filename().generic_string()
    );

    ai_light_reference l_ref{};
    l_ref.session_id_ = l_session.uuid_id_;
    l_ref.ordinal_    = l_next++;
    l_ref.file_path_  = l_ref_dir / fmt::format("ref_{:02}.png", l_ref.ordinal_);
    FSys::rename(l_info.path_, l_ref.file_path_);
    l_ref.width_  = l_info.size_.width;
    l_ref.height_ = l_info.size_.height;
    // 统计（亮度分位/色板/对比/白平衡/削波率）需要 OpenCV 分析层，本轮未落地。
    // 留空 + confidence = 0，会话自动以 prompt_only 降级运行 —— 这是方案 §4.2
    // 本就定义的降级级别，不是临时凑合。
    l_ref.stats_            = nlohmann::json{};
    l_ref.stats_confidence_ = 0.0;
    l_ref.is_outlier_       = false;
    l_added.push_back(l_ref);
  }

  sql_modify_statement_vector_t l_sqls{};
  // 非 const：ORM 的 values() 会回填 uuid_id_ / created_at_（服务端生成字段）
  for (auto& l_ref : l_added)
    l_sqls.emplace_back(insert(l_sql).into<ai_light_reference>().values(l_ref));
  co_await l_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 为 AI 灯光会话 {} 上传了 {} 张参考图", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_added.size()
  );
  co_return in_handle->make_msg(nlohmann::json{{"data", l_added}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(reference_instance_route, delete_) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();

  // 参考图不存在时 get_by_uuid 抛 sqlite_orm_exception{404}，由 http_session_data 统一翻成 404
  auto l_ref     = l_sql.get_by_uuid<ai_light_reference>(id_);
  auto l_session = load_session_checked(person_, l_sql, l_ref.session_id_);
  check_session_mutable(l_session);

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
  auto l_sql = get_sqlite_database();

  // 候选不存在时 get_by_uuid 抛 sqlite_orm_exception{404}，由 http_session_data 统一翻成 404
  auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(id_);
  person_.check_project_access(l_sql.get_by_uuid<ai_light_session>(l_cand.session_id_).project_id_);

  // 未渲染与「文件丢了」是两件事，给不同的话术，省得美术以为是自己的操作错了。
  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::pending, not_found,
      "候选 {} 尚未渲染完成", l_cand.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      l_cand.render_status_ != candidate_render_status::failed, not_found,
      "候选 {} 渲染失败", l_cand.uuid_id_
  );
  DOODLE_CHICK(
      FSys::exists(l_cand.render_png_path_), "候选渲染图不存在 文件 {}", l_cand.render_png_path_.generic_string()
  );

  // 只发 PNG（显示空间）。EXR 是线性数据、体积大且浏览器渲染不了，绝不下发。
  co_return in_handle->make_msg(l_cand.render_png_path_, kitsu::mime_type(l_cand.render_png_path_.extension()));
}

// ===========================================================================
// Spec（闸门 1）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(spec_route, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();

  auto l_session_id = query_uuid(in_handle->url_, "session_id");
  DOODLE_CHICK_HTTP(l_session_id.has_value(), bad_request, "需要 session_id");
  auto l_session = load_session_checked(person_, l_sql, *l_session_id);

  auto l_specs = l_sql.get_ai_light_specs(l_session.uuid_id_);
  co_return in_handle->make_msg(
      nlohmann::json{
          {"data", l_specs},
          {"current_revision", l_specs.empty() ? 0 : l_specs.back().revision_},
      }
  );
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(spec_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_session_id = l_json.value("session_id", uuid{});
  DOODLE_CHICK_HTTP(!l_session_id.is_nil(), bad_request, "缺少 session_id");
  auto l_session = load_session_checked(person_, l_sql, l_session_id);
  check_session_mutable(l_session);

  // 重出方案时允许顺手改提示词（美术看完第一版目标才发现自己没说清是常事）
  if (l_json.contains("prompt_text")) l_json.at("prompt_text").get_to(l_session.prompt_text_);
  DOODLE_CHICK_HTTP(
      !l_session.prompt_text_.empty(), bad_request, "提示词不能为空"
  );
  // 允许带参考图序号做排除（美术看出某张图跑偏了）
  if (l_json.contains("excluded_reference_ids")) {
    // 排除标记写回参考图行，接缝下次取 references 时能看到 is_outlier_
    auto l_excluded = l_json.at("excluded_reference_ids").get<std::vector<uuid>>();
    sql_modify_statement_vector_t l_sqls{};
    for (const auto& l_ref_id : l_excluded) {
      auto l_ref = l_sql.get_by_uuid<ai_light_reference>(l_ref_id);
      DOODLE_CHICK_HTTP(
          l_ref.session_id_ == l_session.uuid_id_, bad_request,
          "参考图 {} 不属于会话 {}", l_ref_id, l_session.uuid_id_
      );
      l_sqls.emplace_back(
          update(l_sql)
              .from<ai_light_reference>()
              .set(
                  c(&ai_light_reference::is_outlier_) = true,
                  c(&ai_light_reference::excluded_reason_) = std::string{"人工排除"}
              )
              .where(c(&ai_light_reference::uuid_id_) == l_ref_id)
      );
    }
    if (!l_sqls.empty()) co_await l_sql.run_sql(std::move(l_sqls));
  }

  // 提示词改了要落库，否则接缝读到的是旧值
  {
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(
        update(l_sql)
            .from<ai_light_session>()
            .set(c(&ai_light_session::prompt_text_) = l_session.prompt_text_)
            .where(c(&ai_light_session::uuid_id_) == l_session.uuid_id_)
    );
    co_await l_sql.run_sql(std::move(l_sqls));
  }

  // 注意：**不**在这里把 suspended 改成 draft。advance_session 的分派本来就同时接受
  // draft 与 suspended（两者都要走 generate_spec），所以不需要也不该提前改状态 ——
  // 提前改会让「接缝失败回滚到入口状态」这条契约变成回滚到 draft，
  // 与文档写的不一致，而状态机最难查的正是这种「回滚落到哪」的偏差。

  // 走到这里参数都合法。接缝未实现时 advance_session 会抛 501 并把状态回滚到入口状态。
  co_await advance_session(l_sql, l_session);

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(spec_confirm_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_session_id = l_json.value("session_id", uuid{});
  DOODLE_CHICK_HTTP(!l_session_id.is_nil(), bad_request, "缺少 session_id");
  auto l_session = load_session_checked(person_, l_sql, l_session_id);

  auto l_spec = l_sql.get_current_ai_light_spec(l_session.uuid_id_);
  DOODLE_CHICK_HTTP(
      l_spec.has_value(), bad_request, "会话 {} 还没有 Spec 可供确认",
      l_session.uuid_id_
  );

  // 幂等：重复确认不报错、不刷新 confirmed_at_（时间戳是「谁在什么时候认的」的证据）
  if (l_spec->human_confirmed_) {
    co_return in_handle->make_msg(
        nlohmann::json{{"spec", *l_spec}, {"session_state", l_session.state_}, {"already_confirmed", true}}
    );
  }

  DOODLE_CHICK_HTTP(
      l_session.state_ == ai_light_session_state::awaiting_spec_confirm, bad_request,
      "会话 {} 处于状态 {}，不在等 Spec 确认", l_session.uuid_id_, magic_enum::enum_name(l_session.state_)
  );

  // 冲突裁决：方案 §6.3 说冲突维度不自动平均，交人工。这里只记录，不改统计。
  conflict_resolution_t l_resolution{};
  if (l_json.contains("conflict_resolution")) l_json.at("conflict_resolution").get_to(l_resolution);

  l_spec->human_confirmed_ = true;
  l_spec->confirmed_by_    = person_.person_.uuid_id_;
  l_spec->confirmed_at_    = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
  l_spec->set_conflict_resolution(l_resolution);

  // 确认后立即进入候选生成（§10.3）。接缝未实现时会抛 501，
  // advance_session 会把会话状态回滚到 awaiting_spec_confirm，Spec 的确认**保留**
  // （确认是人工动作，不该因为下游能力缺失而被撤销）。
  l_session.state_ = ai_light_session_state::generating;

  {
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(
        update(l_sql)
            .from<ai_light_spec>()
            .set(
                c(&ai_light_spec::human_confirmed_) = l_spec->human_confirmed_,
                c(&ai_light_spec::confirmed_by_) = l_spec->confirmed_by_,
                c(&ai_light_spec::confirmed_at_) = l_spec->confirmed_at_,
                c(&ai_light_spec::conflict_resolution_) = l_spec->conflict_resolution_
            )
            .where(c(&ai_light_spec::uuid_id_) == l_spec->uuid_id_)
    );
    l_sqls.emplace_back(
        update(l_sql)
            .from<ai_light_session>()
            .set(c(&ai_light_session::state_) = l_session.state_)
            .where(c(&ai_light_session::uuid_id_) == l_session.uuid_id_)
    );
    co_await l_sql.run_sql(std::move(l_sqls));
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 确认 AI 灯光会话 {} 的 Spec revision {}", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, l_spec->revision_
  );

  co_await advance_session(l_sql, l_session);

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
}

// ===========================================================================
// 迭代（生成一轮候选）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(iterate_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_session_id = l_json.value("session_id", uuid{});
  DOODLE_CHICK_HTTP(!l_session_id.is_nil(), bad_request, "缺少 session_id");
  auto l_session = load_session_checked(person_, l_sql, l_session_id);

  DOODLE_CHICK_HTTP(
      l_session.state_ == ai_light_session_state::generating ||
          l_session.state_ == ai_light_session_state::suspended,
      bad_request,
      "会话 {} 处于状态 {}，不能生成候选（需要 generating 或 suspended）", l_session.uuid_id_,
      magic_enum::enum_name(l_session.state_)
  );

  // 接缝未实现时抛 501，advance_session 已把状态回滚 —— 会话保持可重试。
  co_await advance_session(l_sql, l_session);

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
}

// ===========================================================================
// 人工闸门 2：判断
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(judge_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_session_id = l_json.value("session_id", uuid{});
  DOODLE_CHICK_HTTP(!l_session_id.is_nil(), bad_request, "缺少 session_id");
  auto l_session = load_session_checked(person_, l_sql, l_session_id);

  auto l_iteration = l_sql.get_ai_light_iteration(l_session.uuid_id_, l_session.current_iteration_);
  DOODLE_CHICK_HTTP(
      l_iteration.has_value(), bad_request, "会话 {} 还没有可判断的轮次",
      l_session.uuid_id_
  );

  ai_light_judgement_t l_judgement{};
  l_json.get_to(l_judgement);
  l_judgement.by_ = person_.person_.uuid_id_;
  l_judgement.at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

  // ---- 文本反馈 → 结构化轴（best effort）----
  //
  // 决策 5 把滑块定为默认通道，所以 feedback_axes 是**直接从请求体拿的**，
  // 不经过 LLM。文本是补充通道：能结构化就结构化，不能也不影响这次判断成立。
  // 这正是「接缝未实现时 /judge 依然完全可用」的原因。
  if (!l_judgement.feedback_text_.empty()) {
    try {
      auto l_steps = co_await structure_feedback(l_session.uuid_id_, l_judgement.feedback_text_);
      for (const auto& l_step : l_steps) {
        if (l_step.steps_.empty()) continue;
        // 一个轴可能给多个档位，这里取第一个当作「下一轮的中心值」；
        // 完整档位安排由 generate_candidates 的 axes_ 决定，judge 只传方向。
        l_judgement.feedback_axes_[std::string{magic_enum::enum_name(l_step.axis_)}] = l_step.steps_.front();
      }
    } catch (const doodle_error& in_err) {
      if (in_err.code_status_() != boost::beast::http::status::not_implemented) throw;
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(),
          "AI 灯光会话 {} 的文本反馈未能结构化（接缝未实现），已按纯滑块反馈处理；文本原样留存: {}",
          l_session.uuid_id_, l_judgement.feedback_text_
      );
    }
  }

  // ---- 定稿要先把图落成 preview_file 对象（文件动作），再交给 apply_judgement 同事务落库 ----
  std::optional<preview_file> l_preview_file{};
  if (l_judgement.decision_ == judgement_decision::accept) {
    DOODLE_CHICK_HTTP(
        !l_judgement.selected_candidate_id_.is_nil(), bad_request, "定稿必须指定候选"
    );
    auto l_cand = l_sql.get_by_uuid<ai_light_candidate>(l_judgement.selected_candidate_id_);
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == l_iteration->uuid_id_, bad_request,
        "候选 {} 不属于第 {} 轮", l_cand.uuid_id_, l_iteration->iteration_
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
      magic_enum::enum_name(l_judgement.decision_), l_preview_file.has_value() ? "（已定稿并注册预览文件）" : ""
  );

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_detail);
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
  // 预设的来源由服务端定：客户端传 artist/builtin 会污染「哪些是内置」这个事实
  l_preset.source_     = preset_source::artist;
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
  // source_ / created_by_ 不可改：那是「谁贡献的、是不是内置」的事实

  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(
      update(l_sql)
          .from<ai_light_preset>()
          .set(
              c(&ai_light_preset::name_) = l_preset->name_,
              c(&ai_light_preset::tags_) = l_preset->tags_,
              c(&ai_light_preset::embedding_) = l_preset->embedding_,
              c(&ai_light_preset::embedding_dim_) = l_preset->embedding_dim_,
              c(&ai_light_preset::rig_fragment_) = l_preset->rig_fragment_,
              c(&ai_light_preset::applicable_) = l_preset->applicable_,
              c(&ai_light_preset::stats_target_) = l_preset->stats_target_,
              c(&ai_light_preset::enabled_) = l_preset->enabled_
          )
          .where(c(&ai_light_preset::uuid_id_) == id_)
  );
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

// ===========================================================================
// 镜头页入口（与 /iterate 分工：这里是「建会话 + 跑第一步」）
// ===========================================================================

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(shot_entry_route, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  ai_light_session l_session{};
  l_json.get_to(l_session);
  // 归属由路由与登录态决定，不接受请求体覆盖
  l_session.project_id_ = project_id_;
  l_session.entity_id_  = id_;
  l_session.submitter_  = person_.person_.uuid_id_;
  l_session.state_      = ai_light_session_state::draft;
  l_session.current_iteration_ = 0;
  l_session.failure_reason_    = "";

  DOODLE_CHICK_HTTP(!l_session.task_id_.is_nil(), bad_request, "缺少 task_id");
  DOODLE_CHICK_HTTP(
      !l_session.prompt_text_.empty(), bad_request,
      "缺少 prompt_text —— 提示词是权威意图，不允许空着建会话"
  );
  person_.check_project_access(project_id_);
  if (l_session.title_.empty()) l_session.title_ = l_sql.get_by_uuid<task>(l_session.task_id_).name_;

  {
    sql_modify_statement_vector_t l_sqls{};
    l_sqls.emplace_back(insert(l_sql).into<ai_light_session>().values(l_session));
    co_await l_sql.run_sql(std::move(l_sqls));
  }
  ai_light_suspend_scanner::Get().run();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 从镜头页创建 AI 灯光会话 {}（镜头 {}）", person_.person_.email_,
      person_.person_.get_full_name(), l_session.uuid_id_, id_
  );

  // 会话已经落库了，所以下面这一步**绝不能**让整个请求失败：
  // 一旦返回非 2xx，前端就拿不到 session id，重试只会造出重复会话。
  // 因此只吞「接缝未实现」这一种能力缺口，其余错误照常抛出。
  std::string l_advance_error{};
  try {
    co_await advance_session(l_sql, l_session);
  } catch (const doodle_error& in_err) {
    if (in_err.code_status_() != boost::beast::http::status::not_implemented) throw;
    l_advance_error = in_err.what();
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 已创建，但首步推进未完成: {}", l_session.uuid_id_, l_advance_error
    );
  }

  auto l_detail = build_session_detail(l_sql, l_session.uuid_id_);
  auto l_ret    = nlohmann::json{} = *l_detail;
  if (!l_advance_error.empty()) l_ret["advance_error"] = l_advance_error;
  co_return in_handle->make_msg(l_ret);
}

}  // namespace doodle::http::ai_light
