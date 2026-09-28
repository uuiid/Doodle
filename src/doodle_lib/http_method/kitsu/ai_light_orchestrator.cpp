//
// AI 灯光：轮次编排与状态机实现
//
// 对应 docs/ue_ai_lighting_plan.md §10.3（状态流转）与 §10.4（派发契约）。
// 三个「接缝」（generate_spec / generate_candidates / structure_feedback）在本文件末尾，
// 本轮统一抛 501 —— 契约见 ai_light_orchestrator.h 顶部。
//

#include "ai_light_orchestrator.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/server_task_info.h>

// 注：本文件曾经还包含 app_base.h / boost/asio/co_spawn.hpp / steady_timer.hpp /
// use_awaitable.hpp —— 它们只服务于那个已删除的 4 小时挂起扫描器（常驻后台协程）。
// 过期现在是纯派生标志，这里不再有任何后台任务，所以一并去掉：
// 留着这些 include 会让人以为这儿还跑着一个循环。
#include <doodle_lib/core/entity_path.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <magic_enum/magic_enum.hpp>

#include <chrono>
#include <exception>
#include <string>
#include <string_view>
#include <utility>

namespace doodle::ai_light {

// ORM 的构造助手（insert / update / delete_from / c）与 sqlite_database 的
// 修改语句容器都在 doodle::orm 里，本文件通篇在用。
using namespace orm;

namespace {

using namespace std::chrono_literals;

/// failure_reason_ 的长度上限。失败原因要给人看、要给前端显示，
/// 但 boost 的诊断信息可能带整条调用栈，不截断会把这列撑成日志垃圾场。
constexpr std::size_t g_failure_reason_max = 512;

std::string brief_error(const std::exception& in_err) {
  std::string l_what{in_err.what()};
  if (l_what.size() <= g_failure_reason_max) return l_what;
  l_what.resize(g_failure_reason_max);
  l_what += "...(已截断)";
  return l_what;
}

/// 接缝的统一退出方式。抛 doodle_error 且状态码为 not_implemented，
/// 由 http_session_data.cpp:149-151 映射成 HTTP 501。
[[noreturn]] void throw_not_implemented(std::string_view in_seam, std::string_view in_needs) {
  throw doodle_error{
      boost::beast::http::status::not_implemented,
      fmt::format(
          "AI 灯光接缝 {} 尚未实现：它依赖 {}。方案 §13.1 把这两项排在路由层之后，"
          "本轮的 /spec 与 /iterate 会返回本错误；会话状态保持不变，可以稍后重试。",
          in_seam, in_needs
      )
  };
}

/// 会话产物的落盘基准。
///
/// 为什么要有它：get_shots_auto_lighting_upload_path 返回的是**相对项目根**的路径
/// （entity_path.cpp:425-429 拼的是 "03_Workflow/Shot/EP####/<拼音>"）。
/// 落库与下发 worker 都用绝对路径，免得两边基准不同却都「看起来对」。
struct session_paths_t {
  FSys::path project_root_{};
  episodes episode_{};
  entity_asset_extend_value ground_extend_{};
};

/// 解析会话产物的落盘基准。任何一步缺失都会抛（由 advance_session 统一转成
/// failure_reason_ + 状态回滚）—— 算不出往哪渲染，就不该派发任务。
session_paths_t resolve_session_paths(sqlite_database& in_sql, const ai_light_session& in_session) {
  auto l_shot = in_sql.get_by_uuid<entity>(in_session.entity_id_);
  DOODLE_CHICK_HTTP(
      !l_shot.parent_id_.is_nil(), bad_request,
      "镜头实体 {} 没有父实体，无法推算 AI 灯光产物目录", in_session.entity_id_
  );
  auto l_prj = in_sql.get_by_uuid<project>(in_session.project_id_);

  session_paths_t l_ret{};
  l_ret.project_root_  = l_prj.path_;
  // episodes 是可从 episode 实体构造的元数据类型（先例 up_file.cpp:260）
  l_ret.episode_       = episodes{in_sql.get_by_uuid<entity>(l_shot.parent_id_)};
  l_ret.ground_extend_ = in_sql.get_entity_shot_extend_by_task(in_session.entity_id_);
  return l_ret;
}

/// 会话的参考图统计是否够用。只要有一张图的 stats_ 非空就认为可用 ——
/// 统计落地之前这里恒为 false，于是自动走 prompt_only 降级（方案 §4.2）。
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
          c(&ai_light_session::state_) = in_session.state_,
          c(&ai_light_session::current_iteration_) = in_session.current_iteration_,
          c(&ai_light_session::judgement_due_at_) = in_session.judgement_due_at_,
          c(&ai_light_session::failure_reason_) = in_session.failure_reason_
      )
      .where(c(&ai_light_session::uuid_id_) == in_session.uuid_id_);
}

}  // namespace

// ===========================================================================
// 状态机：推进
// ===========================================================================

boost::asio::awaitable<void> advance_session(sqlite_database& in_sql, ai_light_session in_session) {
  const auto l_entry_state = in_session.state_;

  // Spec 蒸馏的输入是「提示词 + 参考图」（决策 1：参考必有多张）。
  // 参考图清单为空时直接拒绝，**不**静默降级：否则「没传图」和「图传了但统计不可用」
  // 会落进同一个 degrade_level，出问题时看不出是哪一种。
  //
  // 放在 try 之外是有意的：这是入参校验，不该走回滚、也不该往会话上写 failure_reason_ ——
  // 否则美术会在会话页上看到一条看起来像「AI 失败」的提示。
  //
  // 不看 is_outlier_：被人工排除的图仍要交给接缝自行裁决（它拿得到 is_outlier_ 标记），
  // 这与 references_have_stats() 同样不筛该标记的做法一致。
  if (l_entry_state == ai_light_session_state::draft ||
      l_entry_state == ai_light_session_state::awaiting_spec_confirm) {
    DOODLE_CHICK_HTTP(
        !in_sql.get_ai_light_references(in_session.uuid_id_).empty(), bad_request,
        "会话 {} 还没有参考图，无法蒸馏 Spec（决策 1：参考必有多张）", in_session.uuid_id_
    );
  }

  /// 失败收尾：把会话状态退回进入时的状态并记下原因，然后由调用方重新抛出。
  ///
  /// 为什么是「回滚」而不是「置 failed」：failed 是终态，会话一旦进去就没法再用，
  /// 而已经上传的参考图挂在会话上（cascade），重试就意味着重传。接缝未实现、
  /// LLM 超时、输出不合规这类问题都是可重试的，退回原状态既不留卡死的 generating，
  /// 也不丢输入。failed 留给实现方自己判定的不可恢复错误。
  auto l_rollback = [&](const std::string& in_reason) -> boost::asio::awaitable<void> {
    in_session.state_          = l_entry_state;
    in_session.failure_reason_ = in_reason;
    // 时限/挂起时间也跟着回滚：进入时是 awaiting_judgement 的话它们不该被改，
    // 而其它状态下它们本来就该是空的。
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 推进失败（停留在 {}）: {}", in_session.uuid_id_,
        magic_enum::enum_name(l_entry_state), in_reason
    );
    try {
      orm::sql_modify_statement_vector_t l_sqls{};
      l_sqls.emplace_back(make_session_update(in_sql, in_session));
      co_await in_sql.run_sql(std::move(l_sqls));
    } catch (const std::exception& in_err) {
      // 回滚本身失败也不能把原始异常吞掉 —— 只记日志，让原始异常继续往上走。
      SPDLOG_LOGGER_ERROR(
          g_logger_ctrl().get_main_error(), "AI 灯光会话 {} 回滚失败: {}", in_session.uuid_id_, in_err.what()
      );
    }
  };

  std::exception_ptr l_err{};
  std::string l_reason{"未知错误"};

  try {
    // ---------------------------------------------------------------
    // 第一步：draft / awaiting_spec_confirm 且尚无已确认 Spec → 蒸馏出 LightingSpec
    //
    // 两个入口状态，同一个意图：产出（或重出）当前版本的方案。
    // 这不违反「一个状态只能有一个后继步骤」：awaiting_spec_confirm 上还有一个
    // /spec/confirm，但它会**先把状态改成 generating** 再调 advance_session
    // （ai_light.cpp 的 spec_confirm_route），所以落到本分支的只有「重出」这一个意图。
    // ---------------------------------------------------------------
    if (l_entry_state == ai_light_session_state::draft ||
        l_entry_state == ai_light_session_state::awaiting_spec_confirm) {
      auto l_refs = in_sql.get_ai_light_references(in_session.uuid_id_);

      spec_generation_request_t l_req{};
      l_req.session_id_    = in_session.uuid_id_;
      l_req.prompt_text_   = in_session.prompt_text_;
      l_req.references_    = l_refs;
      l_req.degrade_level_ = references_have_stats(l_refs) ? spec_degrade_level::stats_only
                                                           : spec_degrade_level::prompt_only;
      if (auto l_prev = in_sql.get_current_ai_light_spec(in_session.uuid_id_); l_prev.has_value())
        l_req.previous_spec_ = l_prev->to_spec();

      auto l_res = co_await generate_spec(l_req);

      // revision = 现有最大 + 1。get_ai_light_specs 按 revision 升序返回。
      auto l_history  = in_sql.get_ai_light_specs(in_session.uuid_id_);
      const auto l_rev = l_history.empty() ? 1 : l_history.back().revision_ + 1;

      ai_light_spec l_spec{};
      l_spec.session_id_ = in_session.uuid_id_;
      l_spec.revision_   = l_rev;
      l_spec.from_spec(l_res.spec_);
      l_spec.set_evidence(l_res.evidence_);
      l_spec.set_reference_stats(l_res.reference_stats_);
      l_spec.set_conflict_resolution(l_res.conflict_resolution_);
      l_spec.degrade_level_ = l_req.degrade_level_;
      // 新出的方案一律未确认：人工闸门 1 必须重新过一遍。
      l_spec.human_confirmed_ = false;
      l_spec.confirmed_by_    = uuid{};
      l_spec.confirmed_at_    = std::nullopt;

      in_session.state_          = ai_light_session_state::awaiting_spec_confirm;
      in_session.failure_reason_ = "";

      orm::sql_modify_statement_vector_t l_sqls{};
      l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_spec>().values(l_spec));
      l_sqls.emplace_back(make_session_update(in_sql, in_session));
      co_await in_sql.run_sql(std::move(l_sqls));

      SPDLOG_LOGGER_INFO(
          g_logger_ctrl().get_http(), "AI 灯光会话 {} 生成 Spec revision {}（{}）", in_session.uuid_id_, l_rev,
          magic_enum::enum_name(l_req.degrade_level_)
      );
      co_return;
    }

    // ---------------------------------------------------------------
    // 第二步：generating 且 Spec 已确认 → 出候选并派发
    // ---------------------------------------------------------------
    if (l_entry_state == ai_light_session_state::generating) {
      auto l_spec = in_sql.get_current_ai_light_spec(in_session.uuid_id_);
      DOODLE_CHICK_HTTP(
          l_spec.has_value(), bad_request,
          "会话 {} 还没有 Spec，无法生成候选", in_session.uuid_id_
      );
      DOODLE_CHICK_HTTP(
          l_spec->human_confirmed_, bad_request,
          "会话 {} 的 Spec revision {} 尚未经人工确认（闸门 1），不允许生成候选", in_session.uuid_id_,
          l_spec->revision_
      );

      const auto l_iteration = in_session.current_iteration_ + 1;

      candidate_generation_request_t l_req{};
      l_req.session_id_  = in_session.uuid_id_;
      l_req.iteration_   = l_iteration;
      l_req.spec_        = l_spec->to_spec();
      l_req.presets_     = in_sql.get_ai_light_presets(true);
      // warm start：上一轮被选中的候选。首轮没有，由接缝从预设库起手。
      if (auto l_base = in_sql.get_ai_light_selected_candidate(in_session.uuid_id_, in_session.current_iteration_);
          l_base.has_value())
        l_req.base_rig_ = l_base->get_rig();
      // 上一轮人工给的轴调整（滑块通道）。首轮没有。
      if (auto l_prev_it = in_sql.get_ai_light_iteration(in_session.uuid_id_, in_session.current_iteration_);
          l_prev_it.has_value()) {
        auto l_j = l_prev_it->get_judgement();
        if (!l_j.feedback_axes_.empty()) {
          iteration_axes_t l_axes{};
          for (const auto& [l_name, l_value] : l_j.feedback_axes_) {
            auto l_kind = magic_enum::enum_cast<variant_axis_kind>(l_name);
            if (!l_kind.has_value()) continue;  // 未知轴名忽略，不让它把整轮拖垮
            l_axes.axes_.emplace_back(axis_step_t{*l_kind, {l_value}, "人工反馈"});
          }
          l_req.feedback_axes_ = std::move(l_axes);
        }
      }

      auto l_res = co_await generate_candidates(l_req);

      // §6.6 护栏：候选数量与「灯集合一致」是硬约束，不能靠实现方自觉。
      // 数量在这里校验；灯集合一致性需要基线 rig 才谈得上，留到 worker 侧与
      // 候选落库后的回归检查里做（本轮无 worker，先锁住数量这一半）。
      DOODLE_CHICK_HTTP(
          static_cast<std::int32_t>(l_res.rigs_.size()) == g_candidate_count,
          internal_server_error,
          "候选生成接缝返回了 {} 个候选，方案 §6.6 要求恰好 {} 个", l_res.rigs_.size(), g_candidate_count
      );

      auto l_paths = resolve_session_paths(in_sql, in_session);

      ai_light_iteration l_it{};
      l_it.session_id_ = in_session.uuid_id_;
      l_it.iteration_  = l_iteration;
      l_it.spec_id_    = l_spec->uuid_id_;
      l_it.state_      = ai_light_iteration_state::generated;
      l_it.set_axes(l_res.axes_);

      // 分布式任务载荷（方案 §13.1 第 5 项：server_task_info.command_ 已有 JSON 列，
      // 不需要改那张表）。worker 端从 session_id/iteration/candidate_index 自己重算
      // 路径，与服务端落库用的是同一套 entity_path 助手，所以两边必然一致。
      nlohmann::json l_candidates = nlohmann::json::array();
      orm::sql_modify_statement_vector_t l_sqls{};
      for (std::int32_t l_i = 0; l_i < g_candidate_count; ++l_i) {
        auto l_rig = l_res.rigs_.at(static_cast<std::size_t>(l_i));
        // 身份字段由服务端补齐：接缝只负责「灯怎么打」，不负责「这是第几个」。
        l_rig.session_id_      = in_session.uuid_id_;
        l_rig.spec_id_         = l_spec->uuid_id_;
        l_rig.iteration_       = l_iteration;
        l_rig.candidate_index_ = l_i;
        l_rig.rig_id_          = uuid{};

        ai_light_candidate l_cand{};
        l_cand.iteration_id_    = l_it.uuid_id_;
        l_cand.session_id_      = in_session.uuid_id_;
        l_cand.candidate_index_ = l_i;
        l_cand.set_rig(l_rig);
        l_cand.sync_from_rig();  // 轴三列从 rig 同步过来（DB 列是查询权威）
        l_cand.render_png_path_ = l_paths.project_root_ / get_ai_light_candidate_path(
                                                           l_paths.episode_, l_paths.ground_extend_,
                                                           in_session.uuid_id_, l_iteration, l_i, false
                                                       );
        l_cand.render_exr_path_ = l_paths.project_root_ / get_ai_light_candidate_path(
                                                           l_paths.episode_, l_paths.ground_extend_,
                                                           in_session.uuid_id_, l_iteration, l_i, true
                                                       );
        l_cand.render_status_ = candidate_render_status::pending;

        l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_candidate>().values(l_cand));
        l_candidates.push_back(
            nlohmann::json{{"candidate_index", l_i}, {"candidate_id", l_cand.uuid_id_}, {"rig", l_rig}}
        );
      }

      // 任务行。status 保持 submitted，由 computers_assign_task 派发时推进。
      // 「等美术判断」**不是** server_task_info_status —— 那是分布式渲染任务的状态，
      // 两者混用会让工作机的空闲判定错乱（方案 §10.4）。
      server_task_info l_task{};
      l_task.name_       = fmt::format("AI 灯光 {} 轮 {}", in_session.title_.empty() ? "会话" : in_session.title_, l_iteration);
      l_task.status_     = server_task_info_status::submitted;
      l_task.submitter_  = in_session.submitter_;
      l_task.submit_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
      l_task.task_id_    = in_session.task_id_;
      // 用专门的 ai_light 类型，**不要**复用 auto_light ——
      // auto_light 是既有的「跑 UE 装配」管线（distributed_task.cpp:54），
      // 它的 command_ 形状与本轮完全不同，复用会让工作机按错误的解析方式处理。
      l_task.type_       = server_task_info_type::ai_light;
      l_task.command_    = nlohmann::json{
          {"session_id", in_session.uuid_id_},
          {"iteration", l_iteration},
          {"spec_id", l_spec->uuid_id_},
          {"candidates", l_candidates},
          // 基线关卡：worker 复制它再套灯光，不动原关卡（方案 §9）
          {"baseline_level", l_res.rigs_.front().baseline_level_},
      };
      l_it.server_task_info_id_ = l_task.uuid_id_;
      l_it.dispatch_at_         = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

      in_session.current_iteration_ = l_iteration;
      in_session.failure_reason_    = "";
      // 状态**留在 generating**：候选已经生成、任务已经派出，但渲染还没回来。
      // 转 awaiting_judgement 由 mark_iteration_rendered() 负责（§10.3）。

      l_sqls.emplace_back(orm::insert(in_sql).into<server_task_info>().values(l_task));
      l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
      l_sqls.emplace_back(make_session_update(in_sql, in_session));
      co_await in_sql.run_sql(std::move(l_sqls));

      // 派发 + 广播。放在事务之后：任务行已经落库，派发失败也只是「暂时没人领」，
      // computers_assign_task 下次轮询会再捡起来，不会丢。
      co_await http::computers_assign_task::get_instance().run_next_task();
      socket_io::broadcast(socket_io::server_task_info_new_broadcast_t{.server_task_info_id_ = l_task.uuid_id_});

      SPDLOG_LOGGER_INFO(
          g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮 {} 个候选已派发（任务 {}）", in_session.uuid_id_,
          l_iteration, g_candidate_count, l_task.uuid_id_
      );
      co_return;
    }

    // ---------------------------------------------------------------
    // 其它状态：没有可推进的步骤
    // ---------------------------------------------------------------
    DOODLE_CHICK_HTTP(
        false, bad_request, "会话 {} 处于状态 {}，当前没有可推进的步骤",
        in_session.uuid_id_, magic_enum::enum_name(l_entry_state)
    );
  } catch (...) {
    // 注意：C++ 标准禁止在 catch 处理块里 co_await，所以这里只**收集**失败信息，
    // 回滚与重抛都放到 catch 之外。这样也顺带保证了回滚发生在异常对象销毁之前。
    l_err    = std::current_exception();
    l_reason = "未知错误";
    try {
      throw;
    } catch (const std::exception& in_err) {
      l_reason = brief_error(in_err);
    } catch (...) {
      l_reason = "非 std::exception 类型的错误";
    }
  }

  if (l_err) {
    co_await l_rollback(l_reason);
    std::rethrow_exception(l_err);
  }
}

// ===========================================================================
// 状态机：人工闸门 2
// ===========================================================================

boost::asio::awaitable<void> apply_judgement(
    sqlite_database& in_sql, ai_light_session& in_session, ai_light_iteration& in_iteration,
    const ai_light_judgement_t& in_judgement, std::optional<preview_file> in_accepted_preview_file
) {
  DOODLE_CHICK_HTTP(
      in_iteration.session_id_ == in_session.uuid_id_, bad_request,
      "轮次 {} 不属于会话 {}", in_iteration.uuid_id_, in_session.uuid_id_
  );
  DOODLE_CHICK_HTTP(
      in_session.state_ == ai_light_session_state::awaiting_judgement, bad_request,
      "会话 {} 处于状态 {}，不在等人工判断", in_session.uuid_id_, magic_enum::enum_name(in_session.state_)
  );
  DOODLE_CHICK_HTTP(
      in_iteration.state_ != ai_light_iteration_state::judged, bad_request,
      "轮次 {} 已经判断过了", in_iteration.iteration_
  );

  const bool l_needs_candidate = in_judgement.decision_ == judgement_decision::accept ||
                                 in_judgement.decision_ == judgement_decision::continue_iteration;
  DOODLE_CHICK_HTTP(
      l_needs_candidate == !in_judgement.selected_candidate_id_.is_nil(), bad_request,
      "判断 {} 与选中的候选 {} 不匹配", magic_enum::enum_name(in_judgement.decision_),
      in_judgement.selected_candidate_id_
  );
  DOODLE_CHICK_HTTP(
      (in_judgement.decision_ == judgement_decision::accept) == in_accepted_preview_file.has_value(),
      internal_server_error,
      "定稿必须携带已构造好的 preview_file，其余判断不得携带"
  );

  ai_light_session_state l_next_state{};
  switch (in_judgement.decision_) {
    case judgement_decision::accept:             l_next_state = ai_light_session_state::accepted; break;
    case judgement_decision::continue_iteration: l_next_state = ai_light_session_state::generating; break;
    case judgement_decision::reject_all:         l_next_state = ai_light_session_state::rejected; break;
  }

  orm::sql_modify_statement_vector_t l_sqls{};

  // 定稿的 preview_file 与候选的定稿标记必须同事务，见头文件里的说明。
  if (in_accepted_preview_file.has_value())
    l_sqls.emplace_back(orm::insert(in_sql).into<preview_file>().values(*in_accepted_preview_file));

  if (!in_judgement.selected_candidate_id_.is_nil()) {
    auto l_cand = in_sql.get_by_uuid<ai_light_candidate>(in_judgement.selected_candidate_id_);
    // 跨轮误选是最容易犯又最难查的错：第 3 轮点了第 2 轮的图。
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == in_iteration.uuid_id_, bad_request,
        "候选 {} 不属于第 {} 轮", l_cand.uuid_id_, in_iteration.iteration_
    );

    if (in_judgement.decision_ == judgement_decision::accept) {
      // mark_accepted() 会同时写 is_accepted_ 与 accepted_session_id_；
      // 后者是「每会话至多一个定稿」这条约束在库层的载体，不能直接赋值绕过。
      //
      // 刻意**不**同时置 is_selected_：它专指「被选为下一轮基准」，而定稿没有下一轮。
      // 两者混用会让续轮的 warm start 查询拿到一张已定稿的图（见
      // get_ai_light_selected_candidate / get_ai_light_accepted_candidate 的分工）。
      l_cand.mark_accepted();
      l_cand.preview_file_id_ = in_accepted_preview_file->uuid_id_;
    } else {
      l_cand.is_selected_ = true;
    }

    l_sqls.emplace_back(
        orm::update(in_sql)
            .from<ai_light_candidate>()
            .set(
                c(&ai_light_candidate::is_selected_) = l_cand.is_selected_,
                c(&ai_light_candidate::is_accepted_) = l_cand.is_accepted_,
                c(&ai_light_candidate::accepted_session_id_) = l_cand.accepted_session_id_,
                c(&ai_light_candidate::preview_file_id_) = l_cand.preview_file_id_
            )
            .where(c(&ai_light_candidate::uuid_id_) == l_cand.uuid_id_)
    );
  }

  in_iteration.apply_judgement(in_judgement);
  l_sqls.emplace_back(
      orm::update(in_sql)
          .from<ai_light_iteration>()
          .set(
              c(&ai_light_iteration::state_) = in_iteration.state_,
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

boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const std::vector<ai_light_candidate>& in_candidates
) {
  auto l_it = in_sql.get_ai_light_iteration_by_task(in_server_task_info_id);
  DOODLE_CHICK_HTTP(
      l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次",
      in_server_task_info_id
  );
  DOODLE_CHICK_HTTP(
      l_it->state_ != ai_light_iteration_state::judged, bad_request,
      "轮次 {} 已经判断过了，不接受渲染回填", l_it->iteration_
  );

  auto l_session = in_sql.get_by_uuid<ai_light_session>(l_it->session_id_);

  orm::sql_modify_statement_vector_t l_sqls{};
  std::int64_t l_total_ms{0};
  for (const auto& l_cand : in_candidates) {
    // 回填只认属于本轮的候选：worker 报错轮次时宁可少填，也不要串轮。
    DOODLE_CHICK_HTTP(
        l_cand.iteration_id_ == l_it->uuid_id_, bad_request,
        "候选 {} 不属于轮次 {}", l_cand.uuid_id_, l_it->iteration_
    );
    l_total_ms += l_cand.render_ms_;
    l_sqls.emplace_back(
        orm::update(in_sql)
            .from<ai_light_candidate>()
            .set(
                c(&ai_light_candidate::render_status_) = l_cand.render_status_,
                c(&ai_light_candidate::render_png_path_) = l_cand.render_png_path_,
                c(&ai_light_candidate::render_exr_path_) = l_cand.render_exr_path_,
                c(&ai_light_candidate::render_ms_) = l_cand.render_ms_,
                c(&ai_light_candidate::light_count_) = l_cand.light_count_,
                c(&ai_light_candidate::metrics_) = l_cand.metrics_
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
              c(&ai_light_iteration::state_) = l_it->state_,
              c(&ai_light_iteration::render_at_) = l_it->render_at_,
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
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮渲染完成，{} 个候选待判断（整轮 {} ms）",
      l_session.uuid_id_, l_it->iteration_, in_candidates.size(), l_total_ms
  );
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

  auto& l_detail = *l_ret;
  l_detail.references_     = in_sql.get_ai_light_references(in_session_id);
  l_detail.spec_history_   = in_sql.get_ai_light_specs(in_session_id);
  l_detail.current_spec_   = in_sql.get_current_ai_light_spec(in_session_id);
  l_detail.iterations_     = in_sql.get_ai_light_iterations(in_session_id);
  l_detail.candidates_     = in_sql.get_ai_light_candidates_by_session(in_session_id);
  l_detail.accepted_candidate_ = in_sql.get_ai_light_accepted_candidate(in_session_id);

  l_detail.is_waiting_human_ = is_waiting_human(l_detail.session_.state_);
  l_detail.is_judgement_overdue_ = l_detail.session_.is_judgement_overdue(
      chrono::system_zoned_time{chrono::current_zone(), std::chrono::system_clock::now()}
  );
  return l_ret;
}

// ===========================================================================
// 三个接缝（本轮留空）
// ===========================================================================

boost::asio::awaitable<spec_generation_result_t> generate_spec(const spec_generation_request_t& in_request) {
  // 参考图统计（§13.1 第 8 项）与 deepseek_client（第 4 项）都还没落地。
  // 参数在这里被显式引用一次，免得「未使用参数」的告警掩盖了「接缝没实现」这件事。
  (void)in_request;
  throw_not_implemented("generate_spec", "deepseek_client（方案 §13.1 第 4 项）与参考图统计（第 8 项）");
  co_return spec_generation_result_t{};
}

boost::asio::awaitable<candidate_generation_result_t> generate_candidates(
    const candidate_generation_request_t& in_request
) {
  (void)in_request;
  throw_not_implemented("generate_candidates", "deepseek_client（方案 §13.1 第 4 项）与预设库检索");
  co_return candidate_generation_result_t{};
}

boost::asio::awaitable<std::vector<axis_step_t>> structure_feedback(
    const uuid& in_session_id, const std::string& in_feedback_text
) {
  (void)in_session_id;
  (void)in_feedback_text;
  throw_not_implemented("structure_feedback", "deepseek_client（方案 §13.1 第 9 项）");
  co_return std::vector<axis_step_t>{};
}

}  // namespace doodle::ai_light
