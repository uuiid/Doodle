//
// AI 灯光：轮次编排与状态机实现
//
// 对应 docs/ue_ai_lighting_plan.md §10.3（状态流转）与 §10.4（派发契约）。
//
// 本文件只装**状态机**：apply_judgement / mark_iteration_rendered / build_session_detail，
// 候选落库（write_submitted_iteration）与它自己的小助手。不含任何 LLM 接缝调用 ——
// 三条接缝（generate_spec / generate_candidates / structure_feedback）均已归前端，
// 见 docs/ue_ai_lighting_role_downgrade.md。
//

#include "ai_light_orchestrator.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
#include <doodle_lib/http_method/kitsu/auto_task.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/exception/diagnostic_information.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <functional>
#include <magic_enum/magic_enum.hpp>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace doodle::ai_light {

// ORM 的构造助手（insert / update / delete_from / c）与 sqlite_database 的
// 修改语句容器都在 doodle::orm 里，本文件通篇在用。
using namespace orm;

namespace {

using namespace std::chrono_literals;

/// 会话的参考图统计是否够用。只要有一张图分析成功就认为可用。
///
/// `stats_confidence_` 有 0.05 的**地板**（ai_light_stats 的 compute_confidence），
/// 所以「分析成功」与「confidence > 0」是等价的 —— 低置信度是一个数值，不是缺失。
/// 真正的缺失只有一种：统计层还没落地时的空 `stats_`（老库里的历史行）。
///
/// 不看 `is_outlier_`：被人工排除的图仍要交给接缝自行裁决（它拿得到那个标记）。
bool references_have_stats(const std::vector<ai_light_reference>& in_references) {
  return std::any_of(in_references.begin(), in_references.end(), [](const ai_light_reference& in_ref) {
    return !in_ref.stats_.is_null() && in_ref.stats_confidence_ > 0.0;
  });
}

/// 只写会话这几列的更新语句。状态机每处推进都用它，避免各写各的漏列。
orm::sql_modify_statement_t make_session_update(sqlite_database& in_sql, const ai_light_session& in_session) {
  using namespace orm;
  return update(in_sql)
      .from<ai_light_session>()
      .set(
          c(&ai_light_session::state_)             = in_session.state_,
          c(&ai_light_session::current_iteration_) = in_session.current_iteration_,
          c(&ai_light_session::judgement_due_at_)  = in_session.judgement_due_at_,
          c(&ai_light_session::failure_reason_)    = in_session.failure_reason_
      )
      .where(c(&ai_light_session::uuid_id_) == in_session.uuid_id_);
}

}  // namespace

// ===========================================================================
// 状态机
// ===========================================================================
// 服务端不自己演模型：候选由前端算好后经 POST /session/{}/iteration 提交
// （见 ai_light_submit.{h,cpp} 与 docs/ue_ai_lighting_role_downgrade.md）。
//
// generating 的语义是**「等待前端提交本轮候选」**；推进动作是那次提交
// （POST /session/{}/iteration），没有独立的「推进入口」。
//
// 引擎内路线的渲染计划（ai_light_launch_plan_t）只由 write_submitted_iteration 产出。

// ===========================================================================
// 候选落库（角色降级 · 第二梯队）
// ===========================================================================
// 见 ai_light_orchestrator.h 的 submit_iteration_request_t 说明。
boost::asio::awaitable<std::optional<ai_light_launch_plan_t>> write_submitted_iteration(
    sqlite_database& in_sql, const submit_iteration_request_t& in_request, bool in_local_only,
    uuid& out_broadcast_task_id
) {
  // 出参先归零：队列路线之外的出口（引擎内路线、以及任何中途抛错）都不该让调用方
  // 读到一个上一次调用残留的任务号 —— 那会让调用方广播一个无关的旧任务。
  out_broadcast_task_id = uuid{};
  const auto l_iteration = in_request.iteration_;

  // §6.6 护栏：候选不能为空。数量由前端决定，服务端不设上下限；
  // 空数组一定是调用方漏填 —— 走到这里说明闸门被绕过了，按 500 报，不静默放过。
  DOODLE_CHICK_HTTP(
      !in_request.rigs_.empty(), internal_server_error,
      "本轮候选数为 0，方案 §6.6 要求至少一个候选"
  );

  // 装配推导（D50）：失败 / 序列为空 = 本轮不可渲染，抛在这里不落库（调用方记 failure_reason_）。
  //
  // 为什么在这里重算而不由调用方传入：本函数有两个入口 —— 服务端生成（那边已经推过一次）
  // 与前端提交（那边没有）。让入参带上可选装配结果，会把「推没推过」变成调用点各自负责的
  // 判断，而漏传的表现是「本地路线算不出视频路径」这类只在真机上才发现的故障。
  // 重算的代价是一次路径计算 + 4 次 FSys::exists（auto_task.cpp:124-126：全部只做路径计算，
  // 不访问数据库），换来两条路线共用同一段落库逻辑。这是刻意的取舍。
  http::auto_task::shot_render_light_sync_result l_assembly_res{};
  std::string l_assembly_err{};
  if (in_request.task_id_.is_nil()) {
    l_assembly_err = "会话没有关联镜头任务（task_id 为空）";
  } else {
    try {
      l_assembly_res = http::auto_task::shot_render_light_sync_only(in_request.project_id_, in_request.task_id_);
    } catch (const std::exception& in_err) {
      l_assembly_err = in_err.what();
    }
  }
  auto&& l_assembly = l_assembly_res;
  DOODLE_CHICK_HTTP(
      l_assembly_err.empty(), internal_server_error,
      "AI 灯光会话 {} 第 {} 轮：无法推导本镜头的渲染装配，本轮无法渲染（{}）", in_request.session_id_, l_iteration,
      l_assembly_err
  );
  DOODLE_CHICK_HTTP(
      !l_assembly.level_sequence_import_.empty(), internal_server_error,
      "AI 灯光会话 {} 第 {} 轮：镜头装配没有给出 level sequence（{}），本轮无法渲染", in_request.session_id_,
      l_iteration, l_assembly.level_sequence_import_.generic_string()
  );

  ai_light_iteration l_it{};
  // 本轮的 iteration 行 uuid 在这里铸造。降级前它由 advance_session_generating_branch 铸造后
  // 随请求交给接缝；前端提交的路线不调模型，没有那条链路，所以就在这里取。
  l_it.uuid_id_    = core_set::get_set().get_uuid();
  l_it.session_id_ = in_request.session_id_;
  l_it.iteration_  = l_iteration;
  l_it.state_      = ai_light_iteration_state::generated;

  // 两条出口共用的装配（队列载荷 / 本地渲染计划）：形状逐字一致的渲染段。
  // 用 helper 生成「带相对路径的候选」而不是写两遍，是因为这段正是一轮渲染的
  // wire 契约所在 —— 两遍迟早会漂移，而漂移的表现是「本地路线算不出视频路径」这类
  // 只在真机上才发现的故障。
  auto l_out_dir = make_iteration_relative_dir(in_request.session_id_, l_it.uuid_id_);

  // 语句在**局部**批次上组装，函数末尾才组装返回值：任何一条 DOODLE_CHICK 抛错时，
  // 库里一行没动，调用方不可能拿到半截批次。
  orm::sql_modify_statement_vector_t l_sqls{};
  std::vector<ai_light_task_candidate_t> l_payload_candidates{};
  l_payload_candidates.reserve(in_request.rigs_.size());
  for (std::size_t l_i = 0; l_i < in_request.rigs_.size(); ++l_i) {
    auto l_rig = in_request.rigs_.at(l_i);
    // 身份字段由服务端补齐：候选的来源只负责「灯怎么打」，不负责「这是第几个」。
    // 降级后这一点更要紧 —— 前端给的 session_id/iteration 一律不采信。
    l_rig.session_id_      = in_request.session_id_;
    l_rig.iteration_       = l_iteration;
    l_rig.candidate_index_ = static_cast<std::int32_t>(l_i);
    l_rig.rig_id_          = uuid{};
    // baseline_level_ 是只读字段：由上面的装配推导填，前端给什么都不采信。
    l_rig.baseline_level_  = l_assembly.original_map_.generic_string();

    ai_light_candidate l_cand{};
    // uuid 必须在这里就定下来，不能等 ORM 在 insert 时回填：渲染图文件名就是它，
    // nil uuid 会让本轮所有候选全部指向同一个路径（先例：build_accepted_preview_file）。
    l_cand.uuid_id_         = core_set::get_set().get_uuid();
    l_cand.iteration_id_    = l_it.uuid_id_;
    l_cand.session_id_      = in_request.session_id_;
    l_cand.candidate_index_ = l_i;
    l_cand.set_rig(l_rig);
    l_cand.sync_from_rig();  // 索引列从 rig 同步过来（DB 列是查询权威）
    l_cand.render_status_ = candidate_render_status::pending;

    l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_candidate>().values(l_cand));
    // 载荷里带的是**相对路径**（由 make_* 系列 helper 生成，与服务端算落点时用的是同一套
    // 命名规则，只是不带服务端那一长串根）：工作机拼上自己的本地根后交给 UE（D39）。
    // 每候选三样产物（D47）：MRQ 逐帧目录、视频（工作机用 OpenCV 合成）、首帧海报。
    // exr_path_ 本轮不产，保留在载荷里是为了线性交付那条线（D42）。
    l_payload_candidates.push_back(
        ai_light_task_candidate_t{
            static_cast<std::int32_t>(l_i), l_cand.uuid_id_, make_candidate_frames_dir(l_cand.uuid_id_),
            make_candidate_file_name(l_cand.uuid_id_, false), make_candidate_file_name(l_cand.uuid_id_, true),
            make_candidate_movie_file(l_cand.uuid_id_), l_rig
        }
    );
  }

  // 会话行的新值：轮次前进、清空失败原因，状态**留在 generating**
  // （候选已生成、渲染还没回来；转 awaiting_judgement 由 mark_iteration_rendered() 负责，§10.3）。
  auto l_session_next               = in_sql.get_by_uuid<ai_light_session>(in_request.session_id_);
  l_session_next.current_iteration_ = l_iteration;
  l_session_next.failure_reason_    = "";

  // ---------------------------------------------------------------------
  // 引擎内路线（D52/D53）：不建任务行、不派发、不广播，把渲染计划原样返回。
  // ---------------------------------------------------------------------
  if (in_local_only) {
    ai_light_launch_plan_t l_plan{};
    l_plan.session_id_            = in_request.session_id_;
    l_plan.iteration_id_          = l_it.uuid_id_;
    l_plan.iteration_             = l_iteration;
    l_plan.out_dir_               = l_out_dir;
    l_plan.render_map_             = l_assembly.render_map_.generic_string();
    l_plan.level_sequence_        = l_assembly.level_sequence_import_.generic_string();
    l_plan.movie_pipeline_config_ = l_assembly.movie_pipeline_config_.generic_string();
    l_plan.frame_begin_           = l_assembly.begin_time_;
    l_plan.frame_end_             = l_assembly.end_time_;
    // 引擎内路线在发起方本机渲染，所以这里给的是**本机**的工程文件全路径：
    // 工程根(project_dir_) + 工程文件名，由本机自己拼。队列路线则留空，
    // 由工作机用自己的 user_work_root_ 拼（D57：工程是每台机器各自的本地缓存）。
    l_plan.ue_project_path_       = (l_assembly_res.project_dir_ / l_assembly_res.uproject_file_name_).generic_string();
    l_plan.camera_file_path_      = l_assembly.camera_file_path_.generic_string();
    // D57：引擎内路线由发起方本机渲染，装配文件同样要同步到**本机**缓存。
    l_plan.project_dir_           = l_assembly_res.project_dir_;
    l_plan.assembly_files_        = std::move(l_assembly_res.assembly_files_);
    l_plan.candidates_            = std::move(l_payload_candidates);

    // 轮次行的 server_task_info_id_ 留 nil：这条路线**没有**任务行，
    // 回传因此不能按任务号找轮次，改按会话 + 轮次号定位（D54）。
    l_it.server_task_info_id_     = uuid{};

    l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
    l_sqls.emplace_back(make_session_update(in_sql, l_session_next));

    co_await in_sql.run_sql(std::move(l_sqls));

    SPDLOG_LOGGER_INFO(
        g_logger_ctrl().get_http(),
        "AI 灯光会话 {} 第 {} 轮 {} 个候选已落库（引擎内路线，渲染计划已返回，等待发起方本机渲染）",
        in_request.session_id_, l_iteration, in_request.rigs_.size()
    );
    co_return l_plan;
  }

  // ---------------------------------------------------------------------
  // 队列路线：建任务行 → 派发 → 广播，由工作机起 UE 渲染。
  // ---------------------------------------------------------------------
  ai_light_task_payload_t l_payload{};
  l_payload.session_id_            = in_request.session_id_;
  l_payload.iteration_             = l_iteration;
  // 轮次目录名用**轮次 uuid**：候选行靠 iteration_id_ 就能算出落点，不必回查序号。
  l_payload.out_dir_               = l_out_dir;
  l_payload.baseline_level_        = l_assembly.original_map_.generic_string();
  l_payload.render_map_            = l_assembly.render_map_.generic_string();
  l_payload.level_sequence_        = l_assembly.level_sequence_import_.generic_string();
  l_payload.movie_pipeline_config_ = l_assembly.movie_pipeline_config_.generic_string();
  l_payload.frame_begin_           = l_assembly.begin_time_;
  l_payload.frame_end_             = l_assembly.end_time_;
  l_payload.ue_project_path_       = l_assembly_res.uproject_file_name_.generic_string();
  l_payload.camera_file_path_      = l_assembly.camera_file_path_.generic_string();
  // D57：工作机上没有这份 UE 工程缓存，装配素材必须下发同步清单。
  // ue_project_path_ 这里放的是**工程文件名**（不是服务端绝对路径）。
  l_payload.project_dir_           = l_assembly_res.project_dir_;
  l_payload.assembly_files_        = std::move(l_assembly_res.assembly_files_);
  l_payload.candidates_            = std::move(l_payload_candidates);

  // 任务行。status 保持 submitted，由 computers_assign_task 派发时推进。
  server_task_info l_task{};
  l_task.name_   = fmt::format(
      "AI 灯光 {} 轮 {}", l_session_next.title_.empty() ? "会话" : l_session_next.title_, l_iteration
  );
  l_task.status_            = server_task_info_status::submitted;
  l_task.submitter_         = l_session_next.submitter_;
  l_task.submit_time_       = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_task.task_id_           = in_request.task_id_;
  // 用专门的 ai_light 类型，**不要**复用 auto_light —— 它的 command_ 形状与本轮完全不同。
  l_task.type_              = server_task_info_type::ai_light;
  l_task.command_           = l_payload;
  // uuid 要在被引用之前定下来。mark_iteration_rendered() 是**按任务 uuid** 反查轮次的，
  // 这里若读到 nil，轮次的 server_task_info_id_ 就落库为 nil，渲染回传必然 404。
  l_task.uuid_id_           = core_set::get_set().get_uuid();
  l_it.server_task_info_id_ = l_task.uuid_id_;
  l_it.dispatch_at_         = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  l_sqls.emplace_back(orm::insert(in_sql).into<server_task_info>().values(l_task));
  l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
  l_sqls.emplace_back(make_session_update(in_sql, l_session_next));

  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮 {} 个候选已派发（任务 {}）", in_request.session_id_,
      l_iteration, in_request.rigs_.size(), l_task.uuid_id_
  );
  // 队列路线没有本机渲染计划：调用方看到 nullopt 应当按「已派发」处理（不是错误）。
  // 反过来，任务行 uuid 只有这条路线有 —— 把它交给调用方去广播（本函数不做网络副作用）。
  out_broadcast_task_id = l_task.uuid_id_;
  co_return std::nullopt;
}

// ===========================================================================
// 状态机：人工闸门 2
// ===========================================================================

boost::asio::awaitable<void> apply_judgement(
    sqlite_database& in_sql, ai_light_session& in_session, ai_light_iteration& in_iteration,
    const ai_light_judgement_t& in_judgement, std::optional<preview_file> in_accepted_preview_file
) {
  DOODLE_CHICK_HTTP(
      in_iteration.session_id_ == in_session.uuid_id_, bad_request, "轮次 {} 不属于会话 {}", in_iteration.uuid_id_,
      in_session.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      in_session.state_ == ai_light_session_state::awaiting_judgement, bad_request,
      "会话 {} 处于状态 {}，不在等人工判断", in_session.uuid_id_, magic_enum::enum_name(in_session.state_)
  );
  DOODLE_CHICK_HTTP(
      in_iteration.state_ != ai_light_iteration_state::judged, bad_request, "轮次 {} 已经判断过了",
      in_iteration.iteration_
  );

  const bool l_needs_candidate = in_judgement.decision_ == judgement_decision::accept ||
                                 in_judgement.decision_ == judgement_decision::continue_iteration;
  DOODLE_CHICK_HTTP(
      l_needs_candidate == !in_judgement.selected_candidate_id_.is_nil(), bad_request, "判断 {} 与选中的候选 {} 不匹配",
      magic_enum::enum_name(in_judgement.decision_), in_judgement.selected_candidate_id_
  );
  DOODLE_CHICK_HTTP(
      (in_judgement.decision_ == judgement_decision::accept) == in_accepted_preview_file.has_value(),
      internal_server_error, "定稿必须携带已构造好的 preview_file，其余判断不得携带"
  );

  // 三个决定都回到 generating（§15.10 决定 D17）：
  //
  // 会话**没有终态**，所以这里没有分支：三个决定都回到 generating。
  // generating 是唯一能再出一轮的状态（`advance_session` 的 generating 分支 = `POST /iterate`），
  // 而一个轮次判断完之后，唯一的前进方向就是再出一轮 —— 美术对同一个镜头的灯光可以无限轮调整。
  //
  // 三个决定的区别在**动作层**，不在会话状态：定稿要带 preview_file 并落定稿指针，
  // 续轮要有 selected_candidate（下一轮 warm start 的起点），全否则明确不许选候选。
  const ai_light_session_state l_next_state = ai_light_session_state::generating;

  orm::sql_modify_statement_vector_t l_sqls{};

  // 定稿的 preview_file 与候选的定稿标记必须同事务，见头文件里的说明。
  if (in_accepted_preview_file.has_value())
    l_sqls.emplace_back(orm::insert(in_sql).into<preview_file>().values(*in_accepted_preview_file));

  // ---- 定稿是**可移动的单指针**：先把上一次的定稿解除（§15.10 决定 D19）----
  //
  // `accepted_session_id` 上的唯一索引只允许每会话一行定稿，所以第二次定稿必须先
  // 把上一次那行释放掉（置回 NULL），否则下面的 UPDATE 会直接撞唯一约束。
  // 会话没有终态之后这是常态：今天定稿，明天导演改意见，回来接着调、再定稿一张。
  //
  // 旧候选的 preview_file **不删**：它已经作为交付物挂在这个镜头任务上
  // （`ai_light_candidate.preview_file_id_` → `preview_file` 的 FK 是 set_null，
  // 不是 cascade），删掉等于把美术已经交付过的一张图从任务的预览列表里抹掉。
  // 这里只把「当前定稿」这个指针从旧候选上摘下来。
  //
  // 同一次请求重复定稿同一张（幂等重放）时无需解除：判断一下 uuid 相同就跳过，
  // 避免先清后写同一行。
  if (in_judgement.decision_ == judgement_decision::accept) {
    if (auto l_prev = in_sql.get_ai_light_accepted_candidate(in_session.uuid_id_);
        l_prev.has_value() && l_prev->uuid_id_ != in_judgement.selected_candidate_id_) {
      l_prev->clear_accepted();
      l_prev->preview_file_id_ = uuid{};
      l_sqls.emplace_back(
          orm::update(in_sql)
              .from<ai_light_candidate>()
              .set(
                  c(&ai_light_candidate::is_accepted_)         = l_prev->is_accepted_,
                  c(&ai_light_candidate::accepted_session_id_) = l_prev->accepted_session_id_,
                  c(&ai_light_candidate::preview_file_id_)     = l_prev->preview_file_id_
              )
              .where(c(&ai_light_candidate::uuid_id_) == l_prev->uuid_id_)
      );
    }
  }

  if (!in_judgement.selected_candidate_id_.is_nil()) {
    auto l_cand = in_sql.get_by_uuid<ai_light_candidate>(in_judgement.selected_candidate_id_);
    // 跨轮误选是最容易犯又最难查的错：第 3 轮点了第 2 轮的图。
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == in_iteration.uuid_id_, bad_request, "候选 {} 不属于第 {} 轮", l_cand.uuid_id_,
        in_iteration.iteration_
    );

    if (in_judgement.decision_ == judgement_decision::accept) {
      // mark_accepted() 会同时写 is_accepted_ 与 accepted_session_id_；
      // 后者是「每会话至多一个定稿」这条约束在库层的载体，不能直接赋值绕过。
      l_cand.mark_accepted();
      l_cand.preview_file_id_ = in_accepted_preview_file->uuid_id_;
    }
    // 定稿与续轮都要把这张标成「下一轮基准」（§15.10 决定 D20）。
    //
    // 定稿也置 is_selected_：会话没有终态，定稿之后接着调时，下一轮的 warm start
    // （get_ai_light_selected_candidate）应该从**定稿那套 rig** 出发（那是目前最好的一张），
    // 而不是回到 LLM 的原始基线。
    //
    // is_selected_ 与 is_accepted_ 是两个语义（前者 = 下一轮基准，后者 = 当前定稿），
    // 但在同一行上可以同时为真。
    l_cand.is_selected_ = true;

    l_sqls.emplace_back(
        orm::update(in_sql)
            .from<ai_light_candidate>()
            .set(
                c(&ai_light_candidate::is_selected_)         = l_cand.is_selected_,
                c(&ai_light_candidate::is_accepted_)         = l_cand.is_accepted_,
                c(&ai_light_candidate::accepted_session_id_) = l_cand.accepted_session_id_,
                c(&ai_light_candidate::preview_file_id_)     = l_cand.preview_file_id_
            )
            .where(c(&ai_light_candidate::uuid_id_) == l_cand.uuid_id_)
    );
  }

  in_iteration.apply_judgement(in_judgement);
  l_sqls.emplace_back(
      orm::update(in_sql)
          .from<ai_light_iteration>()
          .set(
              c(&ai_light_iteration::state_)     = in_iteration.state_,
              c(&ai_light_iteration::judgement_) = in_iteration.judgement_,
              c(&ai_light_iteration::judged_by_) = in_iteration.judged_by_,
              c(&ai_light_iteration::judged_at_) = in_iteration.judged_at_
          )
          .where(c(&ai_light_iteration::uuid_id_) == in_iteration.uuid_id_)
  );

  // mark_judged 会清掉 4 小时时限，所以挂起扫描器不会再碰这个会话。
  in_session.mark_judged(l_next_state);
  in_session.failure_reason_ = "";
  l_sqls.emplace_back(make_session_update(in_sql, in_session));

  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮判断 {}（操作人 {}）", in_session.uuid_id_,
      in_iteration.iteration_, magic_enum::enum_name(in_judgement.decision_), in_judgement.by_
  );
  co_return;
}

// ===========================================================================
// 状态机：渲染完成回填（worker 里程碑接入）
// ===========================================================================

namespace {

/// 回传的定位方式（D54）：队列路线按任务 uuid 找轮次，引擎内路线没有任务行，
/// 按「会话 + 轮次号」找。两个定位函数合在这里，免得三段代码各写一遍分支。
///
/// 为什么不让调用方先查出轮次再传进来：这三段（mark_iteration_rendered /
/// mark_iteration_failed / apply_render_report）本来就各自查一次库，把定位收在一处
/// 可以让「找不到轮次」的 404 文案只有一份。
std::optional<ai_light_iteration> locate_iteration(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator
) {
  if (!in_server_task_info_id.is_nil()) return in_sql.get_ai_light_iteration_by_task(in_server_task_info_id);
  DOODLE_CHICK_HTTP(
      !in_locator.session_id_.is_nil(), bad_request, "回传没有任务号，也没有会话号，无法定位 AI 灯光轮次"
  );
  return in_sql.get_ai_light_iteration(in_locator.session_id_, in_locator.iteration_);
}

}  // namespace

boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::vector<ai_light_candidate>& in_candidates
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);
  DOODLE_CHICK_HTTP(
      l_it->state_ != ai_light_iteration_state::judged, bad_request, "轮次 {} 已经判断过了，不接受渲染回填",
      l_it->iteration_
  );

  auto l_session = in_sql.get_by_uuid<ai_light_session>(l_it->session_id_);

  orm::sql_modify_statement_vector_t l_sqls{};
  std::int64_t l_total_ms{0};
  for (const auto& l_cand : in_candidates) {
    // 回填只认属于本轮的候选：worker 报错轮次时宁可少填，也不要串轮。
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == l_it->uuid_id_, bad_request, "候选 {} 不属于轮次 {}", l_cand.uuid_id_, l_it->iteration_
    );
    l_total_ms += l_cand.render_ms_;
    // 这里只写状态与实测数值；产物落点按 candidate_render_path() 现算，
    // 与上传 / 取图 / 回传处理用的是同一个函数（D44）。
    l_sqls.emplace_back(
        orm::update(in_sql)
            .from<ai_light_candidate>()
            .set(
                c(&ai_light_candidate::render_status_) = l_cand.render_status_,
                c(&ai_light_candidate::render_ms_)     = l_cand.render_ms_,
                c(&ai_light_candidate::light_count_)   = l_cand.light_count_,
                c(&ai_light_candidate::metrics_)       = l_cand.metrics_
            )
            .where(c(&ai_light_candidate::uuid_id_) == l_cand.uuid_id_)
    );
  }

  l_it->state_           = ai_light_iteration_state::awaiting_judgement;
  l_it->render_at_       = chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_it->render_ms_total_ = l_total_ms;
  l_sqls.emplace_back(
      orm::update(in_sql)
          .from<ai_light_iteration>()
          .set(
              c(&ai_light_iteration::state_) = l_it->state_, c(&ai_light_iteration::render_at_) = l_it->render_at_,
              c(&ai_light_iteration::render_ms_total_) = l_it->render_ms_total_
          )
          .where(c(&ai_light_iteration::uuid_id_) == l_it->uuid_id_)
  );

  // 到这里才起算 4 小时：渲染期间美术本来就没法判断。
  l_session.mark_awaiting_judgement();
  l_session.failure_reason_ = "";
  l_sqls.emplace_back(make_session_update(in_sql, l_session));

  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染完成，{} 个候选待判断（整轮 {} ms）", l_session.uuid_id_,
      l_it->iteration_, in_candidates.size(), l_total_ms
  );
  co_return;
}

// ===========================================================================
// 状态机：工作机渲染回传（§15.12）
// ===========================================================================

namespace {

/// 回传载荷的接缝名。错误消息与诊断日志共用同一个字符串，免得两侧各写一份漂移。
constexpr std::string_view g_render_report_seam{"round_render_report"};

}  // namespace

round_render_report_t parse_round_render_report(const nlohmann::json& in_json) {
  round_render_report_t l_ret{};
  // 就地用 nlohmann 的 contains/at/get<> 硬解析，不走 schema 读取器那层。
  //
  // 成败口径与原先逐条一致：
  //   * 形状不符（缺键 / 类型不对）→ 502 bad_gateway，措辞保持「<接缝>：字段 <路径> …」；
  //   * 语义无效（枚举认不出、可选字段类型不对）→ 静默取默认值，不作废整轮回传。

  auto l_opt_string = [](const nlohmann::json& in_parent, std::string_view in_key) -> std::string {
    if (!in_parent.is_object() || !in_parent.contains(in_key)) return {};
    const auto& l_v = in_parent.at(in_key);
    return l_v.is_string() ? l_v.get<std::string>() : std::string{};
  };
  auto l_opt_number = [](const nlohmann::json& in_parent, std::string_view in_key) -> std::optional<double> {
    if (!in_parent.is_object() || !in_parent.contains(in_key)) return std::nullopt;
    const auto& l_v = in_parent.at(in_key);
    return l_v.is_number() ? std::optional<double>{l_v.get<double>()} : std::nullopt;
  };

  l_ret.error_            = l_opt_string(in_json, "error");

  DOODLE_CHICK_HTTP(
      in_json.is_object() && in_json.contains("candidates") && in_json.at("candidates").is_array(), bad_gateway,
      "{}：字段 candidates 缺失或类型不符合（期望数组）", g_render_report_seam
  );
  const auto& l_candidates = in_json.at("candidates");
  l_ret.candidates_.reserve(l_candidates.size());

  for (std::size_t i = 0; i < l_candidates.size(); ++i) {
    const auto l_base  = fmt::format("candidates[{}]", i);

    // 数组元素必须是对象。带上下标路径再报错，比「解析失败」四个字有用得多。
    const auto& l_item = l_candidates.at(i);
    DOODLE_CHICK_HTTP(
        l_item.is_object(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望对象）",
        g_render_report_seam, l_base
    );

    candidate_render_report_t l_cand{};

    // candidate_id 是严格字段：它对不上任何既存候选时整轮回传都没意义（apply_render_report 会拒）。
    //
    // 手写「取 + 转」的理由：模型输出里从不出现 uuid（候选身份一律由服务端钉），所以没有
    // 专门的读取器。这里把 nlohmann 的解析异常翻成同一套 bad_gateway 措辞，
    // 好让「回传坏了」在美术那边只有一种说法。
    const auto l_id_path = l_base + ".candidate_id";
    DOODLE_CHICK_HTTP(
        l_item.contains("candidate_id") && l_item.at("candidate_id").is_string(), bad_gateway,
        "{}：字段 {} 缺失或类型不符合（期望 uuid 字符串）", g_render_report_seam, l_id_path
    );
    try {
      l_cand.candidate_id_ = l_item.at("candidate_id").get<uuid>();
    } catch (const std::exception& in_err) {
      DOODLE_CHICK_HTTP(
          false, bad_gateway, "{}：字段 {} 不是合法的 uuid（{}）", g_render_report_seam, l_id_path, in_err.what()
      );
    }

    // status 是严格字段：认不出的取值抛 bad_gateway。**刻意不用 NLOHMANN_JSON_SERIALIZE_ENUM
    // 的裸 get<>**：它遇到认不出的字符串不抛，而是静默落到第一个枚举值 —— 那等于把「工作机
    // 回了一个非法状态」变成「这轮渲成功了」，整轮候选都建立在一个错值上。
    DOODLE_CHICK_HTTP(
        l_item.contains("status") && l_item.at("status").is_string(), bad_gateway,
        "{}：字段 {}.status 缺失或类型不符合（期望字符串）", g_render_report_seam, l_base
    );
    {
      const auto l_status_name = l_item.at("status").get<std::string>();
      const auto l_status_v    = magic_enum::enum_cast<candidate_render_status>(l_status_name);
      DOODLE_CHICK_HTTP(
          l_status_v.has_value(), bad_gateway, "{}：字段 {}.status 的值 \"{}\" 不是合法的 {}", g_render_report_seam,
          l_base, l_status_name, magic_enum::enum_type_name<candidate_render_status>()
      );
      l_cand.status_ = *l_status_v;
    }

    // 以下四个是宽松字段（D26：缺失取默认、形状不符丢弃）：
    //   * candidate_index 缺失时按下标推 —— 工作机的回传顺序就是派发顺序；
    //   * render_ms / light_count 缺失记 0。这不会把失败判成成功：判据只有 status_；
    //   * metrics 是诊断数据（给美术看落点、给 VLM 评分用），形状错就丢，不让它把
    //     6 张已经渲出来的图作废。
    l_cand.candidate_index_ =
        static_cast<std::int32_t>(l_opt_number(l_item, "candidate_index").value_or(static_cast<double>(i)));
    l_cand.render_ms_   = static_cast<std::int64_t>(l_opt_number(l_item, "render_ms").value_or(0.0));
    l_cand.light_count_ = static_cast<std::int32_t>(l_opt_number(l_item, "light_count").value_or(0.0));

    if (l_item.contains("metrics")) {
      try {
        l_item.at("metrics").get_to(l_cand.metrics_);
      } catch (const std::exception& in_err) {
        SPDLOG_LOGGER_WARN(
            g_logger_ctrl().get_http(), "AI 灯光回传：候选 {} 的 metrics 不合规，已丢弃（{}）", l_cand.candidate_id_,
            in_err.what()
        );
      }
    }

    l_ret.candidates_.emplace_back(std::move(l_cand));
  }

  return l_ret;
}

boost::asio::awaitable<void> mark_iteration_failed(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::string& in_reason
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);

  auto l_session            = in_sql.get_by_uuid<ai_light_session>(l_it->session_id_);
  l_session.failure_reason_ = in_reason;

  // 只写会话这一行。iteration.state 与候选状态**都不动**（D28）：
  // 失败不是终态，美术再点一次 /iterate 就能重派一轮，而候选行保持 pending 也正好表达
  // 「这一轮没出图」。会话上的 failure_reason_ 是唯一给人看的记录 —— 前端看板靠它
  // 说明「这轮为什么没有图」。
  orm::sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(make_session_update(in_sql, l_session));
  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染失败（会话停留在 {}）：{}", l_session.uuid_id_,
      l_it->iteration_, magic_enum::enum_name(l_session.state_), l_session.failure_reason_
  );
  co_return;
}

boost::asio::awaitable<void> apply_render_report(
    sqlite_database& in_sql, const http::kitsu_ctx_t& in_ctx, const uuid& in_server_task_info_id,
    const ai_light_iteration_locator_t& in_locator, const round_render_report_t& in_report
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);

  // 整轮失败由 error_ 表达（UE 起不来 / 超时 / 载荷非法）；「一个 rendered 都没有」也算失败 ——
  // 否则轮次会被推到 awaiting_judgement，美术面对 6 个不存在的候选视频点「选第 3 个」。
  // 注意：这只是**回报文里**的初判。产物是否真的存在在循环里逐个查（D41），
  // 查完还要再判一次（l_any_rendered_after），否则「6 个都声称 rendered 但产物都不在」
  // 会绕过这里的拦截，把空轮次推给美术。
  if (!in_report.error_.empty() ||
      !std::any_of(
          in_report.candidates_.begin(), in_report.candidates_.end(),
          [](const candidate_render_report_t& in_cand) { return in_cand.status_ == candidate_render_status::rendered; }
      )) {
    co_await mark_iteration_failed(
        in_sql, in_server_task_info_id, in_locator,
        !in_report.error_.empty()
            ? in_report.error_
            : fmt::format("轮次 {} 的 {} 个候选全部渲染失败", l_it->iteration_, in_report.candidates_.size())
    );
    co_return;
  }

  auto l_round = in_sql.get_ai_light_candidates(l_it->uuid_id_);

  std::vector<ai_light_candidate> l_candidates{};
  l_candidates.reserve(in_report.candidates_.size());
  std::size_t l_missing_artifact = 0;

  for (const auto& l_rep : in_report.candidates_) {
    // 候选必须属于**本轮**（只查本轮，不查全库）：别的会话的候选 uuid 混进来必须拒掉，
    // 否则一个坏回传就能改到别人的候选行上。
    const auto l_found = std::find_if(l_round.begin(), l_round.end(), [&](const ai_light_candidate& in_cand) {
      return in_cand.uuid_id_ == l_rep.candidate_id_;
    });
    DOODLE_CHICK_HTTP(
        l_found != l_round.end(), bad_request, "候选 {} 不属于 AI 灯光轮次 {}", l_rep.candidate_id_, l_it->iteration_
    );

    // 产物落点既不在回传里（D27），也不在候选行里（D44）：它就是候选行那三个 uuid 的
    // 纯函数，所以现算一次 —— `in_ctx` 是唯一的运行期输入（布局基准是它的 root_）。
    //
    // 为什么把 ctx 做成参数、而不是在这里取 g_ctx()：test_main 里 g_ctx() 不可用
    // （见 src/test/core/ai_light_paths.cpp:7-10），走全局上下文会把整条成功路径变成
    // 「只有真机能跑」，而那正是回传最容易出错的一段。ctx 可以在栈上构造，于是这段可测。
    ai_light_candidate l_cand = *l_found;
    l_cand.render_status_     = l_rep.status_;
    l_cand.render_ms_         = l_rep.render_ms_;
    l_cand.light_count_       = l_rep.light_count_;
    l_cand.metrics_           = l_rep.metrics_;

    // D41：声称 rendered 就要能取到产物。工作机的上传是**独立一步**（D40），
    // 完全可能出现「UE 写了帧、报告说 rendered，但合成视频或上传那一步失败了」——
    // 这时如果照样落 rendered，前端会拿到 404 的候选视频／海报，而美术的判断通道只有这两样。
    // 两样都查：海报给网格用、视频给判断用，少哪一样这个候选都没法看。
    // 降级成 failed 是**保守**选择：本轮其余候选照常进入判断，只是这个不可选。
    if (l_cand.render_status_ == candidate_render_status::rendered) {
      const FSys::path l_png    = candidate_render_path(in_ctx, l_cand, false);
      const FSys::path l_movie  = candidate_movie_path(in_ctx, l_cand);
      const bool l_png_exists   = FSys::exists(l_png);
      const bool l_movie_exists = FSys::exists(l_movie);
      if (!l_png_exists || !l_movie_exists) {
        ++l_missing_artifact;
        l_cand.render_status_ = candidate_render_status::failed;
        SPDLOG_LOGGER_WARN(
            g_logger_ctrl().get_http(),
            "AI 灯光回传：候选 {}（轮次 {}）声称渲染成功，但产物取不到（海报 {}：{}；视频 {}：{}），已降级为失败",
            l_cand.uuid_id_, l_it->iteration_, l_png.generic_string(), l_png_exists ? "在" : "不在",
            l_movie.generic_string(), l_movie_exists ? "在" : "不在"
        );
      }
    }

    // EXR 是**可选**产物（D42：工作机传不上去不阻断本轮），所以这里不降级，只留一条诊断：
    // 算出来的 EXR 不在时记一笔，让「这一轮的线性图没上来」在日志里看得见。
    const FSys::path l_exr = candidate_render_path(in_ctx, l_cand, true);
    if (!FSys::exists(l_exr)) {
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(), "AI 灯光回传：候选 {}（轮次 {}）的线性 EXR 不在 {}（可选产物，不影响本轮判断）",
          l_cand.uuid_id_, l_it->iteration_, l_exr.generic_string()
      );
    }

    l_candidates.emplace_back(std::move(l_cand));
  }

  // 存在性判定之后**再判一次**「还有没有能看的候选」：全部被上面的降级砍掉时，
  // 这一轮没有任何可判断的东西，必须走失败出口，而不能推成 awaiting_judgement。
  const bool l_any_rendered_after =
      std::any_of(l_candidates.begin(), l_candidates.end(), [](const ai_light_candidate& in_cand) {
        return in_cand.render_status_ == candidate_render_status::rendered;
      });
  if (!l_any_rendered_after) {
    co_await mark_iteration_failed(
        in_sql, in_server_task_info_id, in_locator,
        fmt::format(
            "轮次 {} 的 {} 个候选渲染成功但产物文件缺失（海报或视频），本轮无可判断结果", l_it->iteration_,
            l_missing_artifact
        )
    );
    co_return;
  }

  // 落库与状态流转全部复用原有那一份实现：两条入口（单测 / worker 回传）走同一段事务，
  // 免得「回传路径」与「测试路径」慢慢长成两套语义。
  co_await mark_iteration_rendered(in_sql, in_server_task_info_id, in_locator, l_candidates);
  co_return;
}

// ===========================================================================
// 读模型
// ===========================================================================

std::optional<ai_light_session_detail_t> build_session_detail(sqlite_database& in_sql, const uuid& in_session_id) {
  std::optional<ai_light_session_detail_t> l_ret{};
  try {
    l_ret.emplace();
    l_ret->session_ = in_sql.get_by_uuid<ai_light_session>(in_session_id);
  } catch (const orm::sqlite_orm_exception& in_err) {
    // 会话不存在是**正常的 404**，转成 nullopt 让调用方决定（路由层翻成 404）。
    // 但**只吞 404**：其它 sqlite_orm_exception（约束损坏、库文件坏了…）是真故障，
    // 必须照常上抛成 500。以前这里把整类异常都吞成 nullopt，于是一个真正的库错误
    // 会伪装成「会话不存在」—— 排查方向直接被带偏。
    if (in_err.code_status_() != boost::beast::http::status::not_found) throw;
    SPDLOG_LOGGER_DEBUG(g_logger_ctrl().get_http(), "AI 灯光会话 {} 不存在: {}", in_session_id, in_err.what());
    return std::nullopt;
  }

  auto& l_detail                 = *l_ret;
  l_detail.references_           = in_sql.get_ai_light_references(in_session_id);
  l_detail.iterations_           = in_sql.get_ai_light_iterations(in_session_id);
  l_detail.candidates_           = in_sql.get_ai_light_candidates_by_session(in_session_id);
  l_detail.accepted_candidate_   = in_sql.get_ai_light_accepted_candidate(in_session_id);

  l_detail.is_waiting_human_     = is_waiting_human(l_detail.session_.state_);
  l_detail.is_judgement_overdue_ = l_detail.session_.is_judgement_overdue(
      chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()}
  );
  return l_ret;
}

}  // namespace doodle::ai_light
