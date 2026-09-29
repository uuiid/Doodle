//
// AI 灯光：轮次编排与状态机实现
//
// 对应 docs/ue_ai_lighting_plan.md §10.3（状态流转）与 §10.4（派发契约）。
//
// 三个「接缝」（generate_spec / generate_candidates / structure_feedback）都在本文件末尾：
//   * generate_spec —— 已实现（§15.7）：参考图统计聚合 + 一次火山方舟推理 + 结果映射。
//   * generate_candidates / structure_feedback —— 仍抛 501。
// 接缝的契约（抛即回滚、不得留下非幂等的部分写入、返回值必须自洽）见 ai_light_orchestrator.h 顶部。
//

#include "ai_light_orchestrator.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/configure/static_value.h>

// 注：本文件曾经还包含 app_base.h / boost/asio/co_spawn.hpp / steady_timer.hpp /
// use_awaitable.hpp —— 它们只服务于那个已删除的 4 小时挂起扫描器（常驻后台协程）。
// 过期现在是纯派生标志，这里不再有任何后台任务，所以一并去掉：
// 留着这些 include 会让人以为这儿还跑着一个循环。
// 同理，entity_path.h 与 project.h 也已去掉：它们只服务于那个已删除的
// resolve_session_paths（产物路径改由 kitsu_ctx_t 按会话 uuid 生成，不查库）。
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_method/kitsu/ai_light.h>
#include <doodle_lib/http_method/kitsu/ai_light_stats.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/http_method/seedance2/inference_task.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <map>
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

boost::asio::awaitable<void> advance_session(
    sqlite_database& in_sql, ai_light_session in_session, uuid in_operator_id
) {
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

      // 触发人：本次调用的属性，不是会话的属性 —— 所以不进会话表（加列既逼出迁移、
      // 语义也是错的：同一次灯光工作可能换人重出方案）。
      // 没传时回落到会话提交人；两者都没有就没法解析工作室、也没法计费。
      const auto l_operator = in_operator_id.is_nil() ? in_session.submitter_ : in_operator_id;
      DOODLE_CHICK_HTTP(
          !l_operator.is_nil(), bad_request,
          "会话 {} 无法确定触发人（无 operator_id 且会话无 submitter），无法解析 AI 工作室与计费主体",
          in_session.uuid_id_
      );

      spec_generation_request_t l_req{};
      l_req.session_id_    = in_session.uuid_id_;
      l_req.project_id_    = in_session.project_id_;
      l_req.operator_id_   = l_operator;
      l_req.prompt_text_   = in_session.prompt_text_;
      l_req.references_    = l_refs;
      l_req.degrade_level_ = references_have_stats(l_refs) ? spec_degrade_level::stats_only
                                                           : spec_degrade_level::prompt_only;
      if (auto l_prev = in_sql.get_current_ai_light_spec(in_session.uuid_id_); l_prev.has_value())
        l_req.previous_spec_ = l_prev->to_spec();

      auto l_res = co_await generate_spec(in_sql, l_req);

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
      // 降级级别以**接缝按事实判定的结果**为准，不用调用方那个预判（决定 D4）：
      // 「参考图有 stats 行但置信度全 0」或「全部图被人工排除」时，调用方看
      // references_have_stats() 会以为是 stats_only，而实际聚合出一无所有。
      l_spec.degrade_level_ = l_res.spec_.degrade_level_;
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
          g_logger_ctrl().get_http(), "AI 灯光会话 {} 生成 Spec revision {}（降级级别 {}，参考图 {} 张）",
          in_session.uuid_id_, l_rev, magic_enum::enum_name(l_spec.degrade_level_), l_req.references_.size()
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

      // 产物路径一律由 kitsu_ctx_t 按 uuid 生成（挂在资产根 root_ 下），
      // 不再依赖 episode / shot / 项目路径 —— 所以这里没有「算不出目录」的失败分支。
      // 注意加 http:: 限定：kitsu_ctx_t 在 doodle::http 里，而本文件在 doodle::ai_light。
      auto& l_ctx = g_ctx().get<http::kitsu_ctx_t>();

      ai_light_iteration l_it{};
      // uuid 必须在这里就定下来，不能等 ORM 在 insert 时回填：下面的候选要把
      // 它写进 iteration_id_（mark_iteration_rendered 按这个校验「候选属于本轮」），
      // 而那时 l_it 的 insert 语句还没构造。
      l_it.uuid_id_    = core_set::get_set().get_uuid();
      l_it.session_id_ = in_session.uuid_id_;
      l_it.iteration_  = l_iteration;
      l_it.spec_id_    = l_spec->uuid_id_;
      l_it.state_      = ai_light_iteration_state::generated;
      l_it.set_axes(l_res.axes_);

      // 分布式任务载荷（方案 §13.1 第 5 项：server_task_info.command_ 已有 JSON 列，
      // 不需要改那张表）。worker 端拿 session_id/iteration/candidate_id 自己重算路径，
      // 与服务端落库用的是同一套 kitsu_ctx_t 方法（都挂 root_ 下、文件名都是 uuid），
      // 所以两边必然一致 —— 前提是 worker 配到同一个 root_（pictures/movies 早就这么依赖）。
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
        // uuid 必须在这里就定下来，不能等 ORM 在 insert 时回填：渲染图文件名就是它，
        // nil uuid 会让 6 个候选全部指向同一个路径（先例：build_accepted_preview_file）。
        l_cand.uuid_id_         = core_set::get_set().get_uuid();
        l_cand.iteration_id_    = l_it.uuid_id_;
        l_cand.session_id_      = in_session.uuid_id_;
        l_cand.candidate_index_ = l_i;
        l_cand.set_rig(l_rig);
        l_cand.sync_from_rig();  // 轴三列从 rig 同步过来（DB 列是查询权威）
        l_cand.render_png_path_ =
            l_ctx.get_ai_light_candidate_file(in_session.uuid_id_, l_iteration, l_cand.uuid_id_, false);
        l_cand.render_exr_path_ =
            l_ctx.get_ai_light_candidate_file(in_session.uuid_id_, l_iteration, l_cand.uuid_id_, true);
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
      // 同上：uuid 要在被引用之前定下来。mark_iteration_rendered() 是**按任务 uuid**
      // 反查轮次的（get_ai_light_iteration_by_task），这里若读到 nil，
      // 轮次的 server_task_info_id_ 就落库为 nil，渲染回传必然 404。
      l_task.uuid_id_           = core_set::get_set().get_uuid();
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
// 接缝 1：参考图统计 + 人工提示词 → LightingSpec（方案 §5.3 / §6.1 / §6.3）
// ===========================================================================
//
// 分两层，理由是可测性：单测宿主没有 SSL context（core_set::ctx_ptr）也没有 http logger，
// 真正联网的那一段跑不起来；所以「组信封 / 映射结果」全部做成纯函数（无网络、无数据库），
// 由 ai_light_spec_prompt.cpp 离线覆盖，generate_spec 只负责把两层串起来。

namespace {

/// 证据判定的浮点容差：LLM 把统计数字原样抄回来时不该因为最后一位误差被判成「改了值」。
constexpr double g_evidence_epsilon = 1e-9;

/// 下面的「自由文本枚举」在 spec.h 里是 std::string（取值写在注释里，不是 C++ 枚举）。
/// 把取值收成一处常量，**schema 与校验共用** —— 否则「schema 里写了 subtle、校验却不认它」
/// 这种漂移不会有任何编译期或运行期信号。
constexpr std::array<std::string_view, 4> g_grading_strength_values{"none", "subtle", "moderate", "strong"};
constexpr std::array<std::string_view, 3> g_hardness_values{"hard", "soft", "very_soft"};
constexpr std::array<std::string_view, 3> g_position_bias_values{"side_high", "front_low", "back_high"};
constexpr std::array<std::string_view, 3> g_kelvin_bias_values{"cool", "warm", "neutral"};

/// 模型必须在 evidence 里逐字段交代来源的字段路径 —— 与 map_spec_result 的 put_evidence()
/// 用的是同一套点分路径，将来若要合并模型的自我归因，两边键名天然对齐。
/// 只列**模型可写**的字段：`targets.palette` 与 `reference_stats` 是服务端口径（模型改不了
/// 它们），让模型为这些编理由没有意义。
constexpr std::array<std::string_view, 17> g_evidence_paths{
    "intent.atmosphere",        "intent.time_of_day",          "intent.weather",
    "intent.source_motivation", "intent.character_personality", "intent.story_context",
    "intent.setting",           "targets.contrast",            "targets.white_balance_kelvin",
    "targets.luma_percentiles.p10", "targets.luma_percentiles.p50", "targets.luma_percentiles.p90",
    "targets.clip_rate.high",   "targets.clip_rate.low",       "targets.grading_hints.vignette",
    "targets.grading_hints.lifted_blacks", "targets.grading_hints.bloom",
};

template <std::size_t N>
bool is_one_of(std::string_view in_value, const std::array<std::string_view, N>& in_values) {
  return std::find(in_values.begin(), in_values.end(), in_value) != in_values.end();
}

template <std::size_t N>
nlohmann::json make_string_enum_schema(const std::array<std::string_view, N>& in_values) {
  nlohmann::json l_enum = nlohmann::json::array();
  for (auto&& l_v : in_values) l_enum.emplace_back(l_v);
  return nlohmann::json{{"type", "string"}, {"enum", l_enum}};
}

/// C++ 枚举的 schema。用 magic_enum 的名字生成 —— 枚举值存库走的就是这套名字，
/// 让 schema 成为它的投影，而不是另抄一份字面量。
template <typename E>
nlohmann::json make_enum_schema() {
  nlohmann::json l_enum = nlohmann::json::array();
  for (auto&& l_v : magic_enum::enum_names<E>()) l_enum.emplace_back(l_v);
  return nlohmann::json{{"type", "string"}, {"enum", l_enum}};
}

/// interval_t 的 schema：定长 2 元数组（与 interval_t::from_json 的校验一致，
/// 否则模型给个 3 元数组，入库前会在 from_json 里抛 type_error(302)）。
nlohmann::json make_interval_schema() {
  return nlohmann::json{
      {"type", "array"}, {"items", {{"type", "number"}}}, {"minItems", 2}, {"maxItems", 2}
  };
}

nlohmann::json make_string_array_schema() {
  return nlohmann::json{{"type", "array"}, {"items", {{"type", "string"}}}};
}

const nlohmann::json* opt_object(const nlohmann::json& in_json, const char* in_key) {
  if (!in_json.is_object()) return nullptr;
  const auto l_it = in_json.find(in_key);
  if (l_it == in_json.end() || !l_it->is_object()) return nullptr;
  return &(*l_it);
}

std::optional<std::string> opt_string(const nlohmann::json& in_json, const char* in_key) {
  if (!in_json.is_object()) return std::nullopt;
  const auto l_it = in_json.find(in_key);
  if (l_it == in_json.end() || !l_it->is_string()) return std::nullopt;
  auto l_v = l_it->get<std::string>();
  if (l_v.empty()) return std::nullopt;
  return l_v;
}

std::optional<bool> opt_bool(const nlohmann::json& in_json, const char* in_key) {
  if (!in_json.is_object()) return std::nullopt;
  const auto l_it = in_json.find(in_key);
  if (l_it == in_json.end() || !l_it->is_boolean()) return std::nullopt;
  return l_it->get<bool>();
}

// 注意：interval_t 在这里用聚合初始化 {low, high}，**不要**写成 nlohmann::json{...}
// 那种花括号转换 —— 见 common.h 末尾那条「花括号是列表初始化」的坑。
std::optional<interval_t> opt_interval(const nlohmann::json& in_json, const char* in_key) {
  if (!in_json.is_object()) return std::nullopt;
  const auto l_it = in_json.find(in_key);
  if (l_it == in_json.end() || !l_it->is_array() || l_it->size() != 2) return std::nullopt;
  if (!(*l_it)[0].is_number() || !(*l_it)[1].is_number()) return std::nullopt;
  interval_t l_ret{(*l_it)[0].get<double>(), (*l_it)[1].get<double>()};
  // 模型偶尔把 [high, low] 写反。静默纠正比让下游「落点永远不在区间内」好排查。
  if (l_ret.low_ > l_ret.high_) std::swap(l_ret.low_, l_ret.high_);
  return l_ret;
}

std::vector<std::string> opt_string_array(const nlohmann::json& in_json, const char* in_key) {
  std::vector<std::string> l_ret{};
  if (!in_json.is_object()) return l_ret;
  const auto l_it = in_json.find(in_key);
  if (l_it == in_json.end() || !l_it->is_array()) return l_ret;
  for (const auto& l_v : *l_it)
    if (l_v.is_string()) {
      auto l_s = l_v.get<std::string>();
      if (!l_s.empty()) l_ret.emplace_back(std::move(l_s));
    }
  return l_ret;
}

std::string join_strings(const std::vector<std::string>& in_values, std::string_view in_sep) {
  std::string l_ret{};
  for (std::size_t l_i = 0; l_i < in_values.size(); ++l_i) {
    if (l_i != 0) l_ret += in_sep;
    l_ret += in_values[l_i];
  }
  return l_ret;
}

void put_evidence(
    std::map<std::string, evidence_entry_t>& in_out, const std::string& in_path, evidence_source in_source,
    double in_confidence, const std::string& in_note = {}
) {
  evidence_entry_t l_entry{};
  l_entry.source_     = in_source;
  l_entry.confidence_ = in_confidence;
  l_entry.note_       = in_note;
  in_out[in_path]     = l_entry;
}

std::string rgb_to_hex(const vec3_t& in_color) {
  const auto l_quantize = [](double in_v) {
    return static_cast<int>(std::lround(std::clamp(in_v, 0.0, 1.0) * 255.0));
  };
  return fmt::format("#{:02x}{:02x}{:02x}", l_quantize(in_color.x_), l_quantize(in_color.y_), l_quantize(in_color.z_));
}

/// 「提示词未提及 + 参考冲突 → 不设目标」这一支由聚合器完成：冲突维度不写进硬目标。
std::string describe_targets(const lighting_targets_t& in_targets) {
  std::string l_out{};
  l_out += fmt::format(
      "  contrast=[{}, {}]  white_balance_kelvin=[{}, {}]\n", in_targets.contrast_.low_, in_targets.contrast_.high_,
      in_targets.white_balance_kelvin_.low_, in_targets.white_balance_kelvin_.high_
  );
  l_out += fmt::format(
      "  luma p10=[{}, {}]  p50=[{}, {}]  p90=[{}, {}]\n", in_targets.luma_percentiles_.p10_.low_,
      in_targets.luma_percentiles_.p10_.high_, in_targets.luma_percentiles_.p50_.low_,
      in_targets.luma_percentiles_.p50_.high_, in_targets.luma_percentiles_.p90_.low_,
      in_targets.luma_percentiles_.p90_.high_
  );
  l_out += fmt::format(
      "  clip_rate high=[{}, {}]  low=[{}, {}]\n", in_targets.clip_rate_.high_.low_, in_targets.clip_rate_.high_.high_,
      in_targets.clip_rate_.low_.low_, in_targets.clip_rate_.low_.high_
  );
  l_out += fmt::format(
      "  grading_hints: vignette={} lifted_blacks={} bloom={}\n", in_targets.grading_hints_.vignette_,
      in_targets.grading_hints_.lifted_blacks_ ? "true" : "false", in_targets.grading_hints_.bloom_
  );
  return l_out;
}

}  // namespace

std::string build_spec_instructions() {
  return std::string{R"SPEC(你是电影灯光指导。把一个人工提示词与多张参考图的统计，蒸馏成一份可执行的灯光目标（LightingSpec）。

【职责边界：你决定什么、不决定什么】
你决定：灯的数量、角色、类型、相对方位；环境光 / 雾 / 天空 / 曝光 / 调色的整体倾向；增删改哪些灯（结构决策）。
你不决定：最终精确强度 / 色温 / 距离的数值；直接写 UE API 调用或代码；逐帧关键帧的精确数值；预测渲染结果。
绝对禁止：改变场景几何或材质。你的可改范围只有「灯光 + 环境光/雾/天空 + 后处理/调色」。

【冲突规则：人工提示词优先于参考图】
参考只提供环境与场景的结构与数值，提示词提供意图与语义。
- 提示词与参考一致 → 直接合并，置信度提高。
- 提示词明确、参考冲突 → 提示词胜出。
- 提示词未提及、参考明确 → 参考胜出。
- 提示词未提及、参考冲突 → 该维度不要设为硬目标，把它列进 conflicts，交人工裁决。

【人物角色性格 → 灯光语言映射（提示词里最有价值、也最容易落不到灯上的字段）】
| 性格维度 | key:fill 比 | 硬软 | 光位倾向 | 色温倾向 |
| 阴郁 / 内向 | 高（6:1–10:1） | 硬 | 侧高、少正面 | 偏冷 |
| 开朗 / 外向 | 低（2:1–3:1） | 软 | 正面偏上 | 偏暖 |
| 威严 / 压迫 | 很高（10:1+） | 硬 | 顶光 / 底光 | 中性偏冷 |
| 温柔 / 脆弱 | 很低（1.5:1–2:1） | 很软 | 正面、包覆 | 暖 |
| 危险 / 不稳定 | 高且不稳定 | 硬 | 侧后、明暗交界 | 冷暖冲突 |
| 神圣 / 超然 | 中 | 很软 | 顶光 | 冷白 |
提示词里出现角色性格时，必须按这张表落成 character_personality[].lighting_language 的四个字段，不要停留在形容词。
key_fill_ratio 写成 [low, high]（如阴郁内向写 [6, 10]）。
hardness 只能是 hard / soft / very_soft；position_bias 只能是 side_high / front_low / back_high；
kelvin_bias 只能是 cool / warm / neutral。

【单位与格式口径】
- 对比度 = 显示空间（sRGB）亮度分位差 p90 − p10，不是标准差。
- 色温用 Kelvin。
- 所有区间都是长度为 2 的数组 [low, high]，且 low <= high。
- 亮度分位 p10 / p50 / p90 是显示空间的归一化值（0 = 黑，1 = 白）。
- 削波率 clip_rate 是 [0, 1] 的比例。
- grading_hints.vignette / bloom 只能是 none / subtle / moderate / strong。

【输出要求】
只输出符合给定 JSON Schema 的 JSON。不要输出解释文字，不要用 Markdown 代码块。
调色板（palette）与 ownership 不在 schema 里：颜色只能来自参考图统计，不接受你编造数值。
)SPEC"};
}

nlohmann::json build_spec_json_schema() {
  const auto l_interval     = make_interval_schema();
  const auto l_string_array = make_string_array_schema();

  nlohmann::json l_language{};
  l_language["type"]                 = "object";
  l_language["additionalProperties"] = false;
  l_language["properties"]           = nlohmann::json{
      {"key_fill_ratio", l_interval},
      {"hardness", make_string_enum_schema(g_hardness_values)},
      {"position_bias", make_string_enum_schema(g_position_bias_values)},
      {"kelvin_bias", make_string_enum_schema(g_kelvin_bias_values)},
  };
  l_language["required"] = nlohmann::json::array({"key_fill_ratio", "hardness", "position_bias", "kelvin_bias"});

  nlohmann::json l_personality{};
  l_personality["type"]                 = "object";
  l_personality["additionalProperties"] = false;
  l_personality["properties"]           = nlohmann::json{
      {"subject", {{"type", "string"}}},
      {"trait", {{"type", "string"}}},
      {"lighting_language", l_language},
  };
  l_personality["required"] = nlohmann::json::array({"subject", "trait", "lighting_language"});

  nlohmann::json l_intent{};
  l_intent["type"]                 = "object";
  l_intent["additionalProperties"] = false;
  l_intent["properties"]           = nlohmann::json{
      {"atmosphere", l_string_array},
      {"time_of_day", {{"type", "string"}}},
      {"weather", {{"type", "string"}}},
      {"source_motivation", l_string_array},
      {"character_personality", nlohmann::json{{"type", "array"}, {"items", l_personality}}},
      {"story_context", {{"type", "string"}}},
      {"setting", {{"type", "string"}}},
  };
  l_intent["required"] = nlohmann::json::array(
      {"atmosphere", "time_of_day", "weather", "source_motivation", "character_personality", "story_context", "setting"}
  );

  nlohmann::json l_luma{};
  l_luma["type"]                 = "object";
  l_luma["additionalProperties"] = false;
  l_luma["properties"] = nlohmann::json{{"p10", l_interval}, {"p50", l_interval}, {"p90", l_interval}};
  l_luma["required"]   = nlohmann::json::array({"p10", "p50", "p90"});

  nlohmann::json l_clip{};
  l_clip["type"]                 = "object";
  l_clip["additionalProperties"] = false;
  l_clip["properties"]           = nlohmann::json{{"high", l_interval}, {"low", l_interval}};
  l_clip["required"]             = nlohmann::json::array({"high", "low"});

  nlohmann::json l_grading{};
  l_grading["type"]                 = "object";
  l_grading["additionalProperties"] = false;
  l_grading["properties"]           = nlohmann::json{
      {"vignette", make_string_enum_schema(g_grading_strength_values)},
      {"lifted_blacks", {{"type", "boolean"}}},
      {"bloom", make_string_enum_schema(g_grading_strength_values)},
  };
  l_grading["required"] = nlohmann::json::array({"vignette", "lifted_blacks", "bloom"});

  // 刻意没有 palette / ownership / space：颜色与所有权划分是服务端的口径，
  // 让模型每个字段都能自由填只会引入无法校验的分叉（§6.6 护栏）。
  nlohmann::json l_targets{};
  l_targets["type"]                 = "object";
  l_targets["additionalProperties"] = false;
  l_targets["properties"]           = nlohmann::json{
      {"contrast", l_interval},
      {"white_balance_kelvin", l_interval},
      {"luma_percentiles", l_luma},
      {"clip_rate", l_clip},
      {"grading_hints", l_grading},
  };
  l_targets["required"] =
      nlohmann::json::array({"contrast", "white_balance_kelvin", "luma_percentiles", "clip_rate", "grading_hints"});

  nlohmann::json l_evidence_entry{};
  l_evidence_entry["type"]                 = "object";
  l_evidence_entry["additionalProperties"] = false;
  l_evidence_entry["properties"]           = nlohmann::json{
      {"source", make_enum_schema<evidence_source>()},
      {"confidence", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 1.0}}},
      {"note", {{"type", "string"}}},
  };
  l_evidence_entry["required"] = nlohmann::json::array({"source", "confidence", "note"});

  // evidence 是**固定键**字典，键就是 g_evidence_paths 那 17 条字段路径。
  // 曾经这里是 `additionalProperties = <条目 schema>` 的自由字典，那是错的：
  // 严格结构化输出要求每个 object 都 additionalProperties:false，自由字典会被判成
  // 不合规（这类 schema 最典型的真机 400）。固定键不需要改语义 —— evidence 的键本来
  // 就是有限集合（可写字段就那么些），只是当初误以为"枚举不完"。
  nlohmann::json l_evidence_props = nlohmann::json::object();
  nlohmann::json l_evidence_req   = nlohmann::json::array();
  for (auto&& l_path : g_evidence_paths) {
    const auto l_key        = std::string{l_path};
    l_evidence_props[l_key] = l_evidence_entry;
    l_evidence_req.emplace_back(l_key);
  }
  nlohmann::json l_evidence{};
  l_evidence["type"]                 = "object";
  l_evidence["additionalProperties"] = false;
  l_evidence["properties"]           = std::move(l_evidence_props);
  l_evidence["required"]             = std::move(l_evidence_req);

  // 全字段 required + strict：OpenAI / 方舟的严格结构化输出基本都要求 required 列全。
  // 代价是「不设目标」不能用「省略字段」表达 —— 那种情况请把该维度写进 conflicts，
  // 并在对应字段上照抄统计值（人工闸门 1 看到的正是这份 conflicts）。
  // 真机若仍 400，§15.2 的降级方案是换成 SDK 的嵌套 text.format 形状。
  nlohmann::json l_root{};
  l_root["type"]                 = "object";
  l_root["additionalProperties"] = false;
  l_root["properties"]           = nlohmann::json{
      {"intent", l_intent},
      {"targets", l_targets},
      {"evidence", l_evidence},
      {"conflicts", l_string_array},
  };
  l_root["required"] = nlohmann::json::array({"intent", "targets", "evidence", "conflicts"});
  return l_root;
}

std::string build_spec_input_text(
    const spec_generation_request_t& in_request, const aggregated_reference_stats_t& in_aggregated
) {
  std::string l_out{};

  l_out += "【人工提示词（权威意图）】\n";
  if (in_request.prompt_text_.empty()) {
    l_out += "(美术未填写提示词 —— 只能依据参考图统计推断；请把 intent 里靠猜的字段置信度调低)\n";
  } else {
    l_out += in_request.prompt_text_;
    l_out += "\n";
  }

  const auto l_excluded =
      static_cast<std::size_t>(std::count_if(in_request.references_.begin(), in_request.references_.end(), [](const auto& in_ref) {
        return in_ref.is_outlier_;
      }));
  l_out += fmt::format(
      "\n【参考图逐图统计（显示空间 sRGB）】共 {} 张，其中 {} 张已被人工排除"
      "（不进聚合，也不发图给你；仅在此列出以便你理解被否掉的是什么）\n",
      in_request.references_.size(), l_excluded
  );
  for (const auto& l_ref : in_request.references_) {
    const auto l_stats = l_ref.get_stats();
    l_out += fmt::format(
        "- #{} {}x{} confidence={:.2f}{}\n", l_ref.ordinal_, l_ref.width_, l_ref.height_, l_ref.stats_confidence_,
        l_ref.is_outlier_ ? fmt::format("  已人工排除（{}）", l_ref.excluded_reason_) : std::string{}
    );
    // 判据与匿名命名空间的 references_have_stats() 一致：get_stats() 返回的是值类型，
    // 统计缺失时得到的是全零的 reference_image_stats_t，无法与「真的全零」区分，
    // 所以可用性只能由 stats_ 是否为 null 与 stats_confidence_ 共同判定。
    if (l_ref.stats_.is_null() || l_ref.stats_confidence_ <= 0.0) {
      l_out += "  该图没有可用统计（分析失败或未分析）\n";
      continue;
    }
    l_out += fmt::format(
        "  luma p10={:.4f} p50={:.4f} p90={:.4f}  contrast={:.4f}  K={:.0f}\n",
        l_stats.luma_percentiles_.p10_, l_stats.luma_percentiles_.p50_, l_stats.luma_percentiles_.p90_, l_stats.contrast_,
        l_stats.white_balance_kelvin_
    );
    l_out += fmt::format(
        "  clip high={:.4f} low={:.4f}  retouch={:.2f}\n", l_stats.clip_rate_.high_, l_stats.clip_rate_.low_,
        l_stats.retouch_score_
    );
    if (!l_stats.palette_.empty()) {
      l_out += "  主色（按占比降序，前 3）: ";
      const auto l_take = std::min<std::size_t>(std::size_t{3}, l_stats.palette_.size());
      for (std::size_t l_i = 0; l_i < l_take; ++l_i)
        l_out += fmt::format("{}{:.0f}%", rgb_to_hex(l_stats.palette_[l_i].color_), l_stats.palette_[l_i].share_ * 100.0);
      l_out += "\n";
    }
  }

  l_out += "\n【聚合目标（多图中位数 + 极差区间）】\n";
  if (in_aggregated.stats_.per_dimension_confidence_.empty()) {
    l_out += "没有可用统计 —— 请完全依据提示词推断 targets，不要编造精确定位数字。\n";
  } else {
    l_out += describe_targets(in_aggregated.targets_);
    l_out += fmt::format(
        "  参与聚合的图 {} 张，聚合方式 {}（区间取极差，不是均值±容差）\n", in_aggregated.stats_.image_count_,
        in_aggregated.stats_.aggregation_
    );
    l_out += "  逐维置信度:";
    for (const auto& [l_key, l_value] : in_aggregated.stats_.per_dimension_confidence_)
      l_out += fmt::format(" {}={:.2f}", l_key, l_value);
    l_out += "\n";
    if (!in_aggregated.stats_.conflicts_.empty())
      l_out += fmt::format(
          "  冲突维度（参考图之间互相矛盾 → 列进 conflicts，不要当硬目标）: {}\n",
          join_strings(in_aggregated.stats_.conflicts_, ", ")
      );
    if (!in_aggregated.suspected_outlier_ids_.empty()) {
      l_out += "  疑似外点（仅供你判断参考，尚未排除）: ";
      for (const auto& l_id : in_aggregated.suspected_outlier_ids_) l_out += boost::uuids::to_string(l_id) + " ";
      l_out += "\n";
    }
    l_out += "  调色板（只能照抄，不接受改写）: ";
    for (const auto& l_entry : in_aggregated.targets_.palette_)
      l_out += fmt::format(
          "{}[{:.0f}%-{:.0f}%] ", rgb_to_hex(l_entry.color_), l_entry.share_.low_ * 100.0, l_entry.share_.high_ * 100.0
      );
    l_out += "\n";
  }

  if (in_request.previous_spec_.has_value()) {
    l_out += "\n【上一版方案（美术对上一版不满意，请重出）】\n";
    l_out += "上一版 intent:\n";
    l_out += fmt::format(
        "  atmosphere=[{}] time_of_day={} weather={}\n  source_motivation=[{}] setting={}\n  story_context={}\n",
        join_strings(in_request.previous_spec_->intent_.atmosphere_, ", "),
        in_request.previous_spec_->intent_.time_of_day_, in_request.previous_spec_->intent_.weather_,
        join_strings(in_request.previous_spec_->intent_.source_motivation_, ", "),
        in_request.previous_spec_->intent_.setting_, in_request.previous_spec_->intent_.story_context_
    );
    l_out += "上一版 targets:\n";
    l_out += describe_targets(in_request.previous_spec_->targets_);
    l_out +=
        "除非提示词或统计明确要求改动，否则请与上一版保持一致 —— 美术不满意的是具体某处，"
        "大范围重写会让上一轮的判断信息作废。\n";
  }

  return l_out;
}

nlohmann::json build_spec_request(
    const spec_generation_request_t& in_request, const aggregated_reference_stats_t& in_aggregated
) {
  nlohmann::json l_req{};
  l_req["instructions"]       = build_spec_instructions();
  l_req["input_text"]         = build_spec_input_text(in_request, in_aggregated);
  l_req["image_detail"]       = "high";
  l_req["temperature"]        = 0.2;
  l_req["json_schema"]        = build_spec_json_schema();
  l_req["json_schema_name"]   = "lighting_spec";
  l_req["json_schema_strict"] = true;
  // 刻意**不发** max_output_tokens：schema 要求全字段 required（含 17 条 evidence），输出长度
  // 不可预估，一旦被截断 result_json_ 就变成不可解析的半个 JSON —— 那个失败面与「平铺形状
  // 被拒」混在一起根本分不出来。上限交给模型/账号侧决定。
  // 不发 thinking：缺省就是 disabled，而结构化任务开 thinking 会引入
  // 「output[0] 是 reasoning 项」那条坑（§15.6 第 2 条）。

  // images 填**站点内相对路径**，由 to_public_url 补公网前缀 + 短时效签名；
  // 绝对磁盘路径（file_path_）绝不能外发。被人工排除的图不发。
  std::vector<std::string> l_images{};
  for (const auto& l_ref : in_request.references_) {
    if (l_ref.is_outlier_) continue;
    // reference_image_url 定义在路由头 ai_light.h 的 doodle::http::ai_light 命名空间里。
    l_images.emplace_back(http::ai_light::reference_image_url(l_ref.uuid_id_));
  }
  if (!l_images.empty()) l_req["images"] = l_images;
  return l_req;
}

spec_generation_result_t map_spec_result(
    const nlohmann::json& in_result_json, const aggregated_reference_stats_t& in_aggregated
) {
  DOODLE_CHICK_HTTP(
      in_result_json.is_object(), bad_gateway, "LLM 未返回 JSON 对象（{}），无法映射为 LightingSpec",
      in_result_json.is_null() ? std::string{"null —— 没有按 json_schema 输出"} : in_result_json.type_name()
  );

  spec_generation_result_t l_ret{};
  auto& l_spec = l_ret.spec_;

  // ---- 服务端钉死：模型无权决定的部分 ----
  // schema 里根本没有这些键，所以「钉死」= 不读模型输出，只写服务端口径。
  // 身份字段（spec_id_ / session_id_）由调用方填：spec_id_ 派生自 uuid_id_，
  // session_id_ 是会话锚点，都不属于 LLM 的契约。
  l_spec.schema_version_ = std::string{g_spec_schema_version};
  l_spec.targets_        = in_aggregated.targets_;  // 区间 + palette + ownership 全来自统计
  l_spec.targets_.space_ = "display_srgb";
  l_ret.reference_stats_ = in_aggregated.stats_;
  l_spec.reference_stats_ = in_aggregated.stats_;
  l_spec.human_confirmed_ = false;
  l_spec.confirmed_by_    = uuid{};
  l_spec.confirmed_at_    = std::nullopt;

  // 降级级别以**事实**为准：聚合器一个维度的置信度都没给出 = 没有可用统计。
  // 不用调用方那个 references_have_stats() 的预判 —— 它只看 stats_confidence_ > 0，
  // 而「全部图被人工排除」「stats 行存在但都是空壳」都会在这里翻案（决定 D4）。
  const bool l_has_stats = !in_aggregated.stats_.per_dimension_confidence_.empty();
  l_spec.degrade_level_  = l_has_stats ? spec_degrade_level::stats_only : spec_degrade_level::prompt_only;

  put_evidence(
      l_ret.evidence_, "targets.palette", evidence_source::reference_stats, l_has_stats ? 0.8 : 0.3,
      "调色板只来自参考图统计，不接受模型编造"
  );
  put_evidence(
      l_ret.evidence_, "reference_stats", evidence_source::reference_stats,
      in_aggregated.stats_.per_dimension_confidence_.empty()
          ? 0.0
          : [&] {
              double l_sum = 0.0;
              for (const auto& [l_key, l_value] : in_aggregated.stats_.per_dimension_confidence_) l_sum += l_value;
              return l_sum / static_cast<double>(in_aggregated.stats_.per_dimension_confidence_.size());
            }(),
      fmt::format("{} 张图，聚合方式 {}", in_aggregated.stats_.image_count_, in_aggregated.stats_.aggregation_)
  );

  // ---- 模型可覆盖：先记下被改动的维度，供 §6.3 的裁决留痕 ----
  std::vector<std::string> l_overridden{};

  // contrast 等区间的 evidence 判定是**机械的**：与统计同名字段逐元素比较，
  // 相等 → 模型只是把统计抄了回来（reference_stats）；不等 → 按「提示词优先」归因于提示词。
  const auto l_apply_interval =
      [&](const nlohmann::json& in_src, const char* in_key, interval_t& in_out, const std::string& in_path) {
        const auto l_llm = opt_interval(in_src, in_key);
        if (!l_llm.has_value()) return;
        const bool l_same = std::abs(l_llm->low_ - in_out.low_) < g_evidence_epsilon &&
                            std::abs(l_llm->high_ - in_out.high_) < g_evidence_epsilon;
        in_out = *l_llm;
        if (l_same) {
          put_evidence(l_ret.evidence_, in_path, evidence_source::reference_stats, 0.8, "与参考图统计一致");
        } else {
          put_evidence(
              l_ret.evidence_, in_path, evidence_source::prompt, 0.7,
              "与参考图统计不一致：按 §6.3「提示词优先于参考」采用模型给出的值"
          );
          l_overridden.emplace_back(in_path);
        }
      };

  if (const auto* l_targets_json = opt_object(in_result_json, "targets"); l_targets_json != nullptr) {
    l_apply_interval(*l_targets_json, "contrast", l_spec.targets_.contrast_, "targets.contrast");
    l_apply_interval(
        *l_targets_json, "white_balance_kelvin", l_spec.targets_.white_balance_kelvin_, "targets.white_balance_kelvin"
    );
    if (const auto* l_luma = opt_object(*l_targets_json, "luma_percentiles"); l_luma != nullptr) {
      l_apply_interval(*l_luma, "p10", l_spec.targets_.luma_percentiles_.p10_, "targets.luma_percentiles.p10");
      l_apply_interval(*l_luma, "p50", l_spec.targets_.luma_percentiles_.p50_, "targets.luma_percentiles.p50");
      l_apply_interval(*l_luma, "p90", l_spec.targets_.luma_percentiles_.p90_, "targets.luma_percentiles.p90");
    }
    if (const auto* l_clip = opt_object(*l_targets_json, "clip_rate"); l_clip != nullptr) {
      l_apply_interval(*l_clip, "high", l_spec.targets_.clip_rate_.high_, "targets.clip_rate.high");
      l_apply_interval(*l_clip, "low", l_spec.targets_.clip_rate_.low_, "targets.clip_rate.low");
    }
    if (const auto* l_grading = opt_object(*l_targets_json, "grading_hints"); l_grading != nullptr) {
      // 自由文本枚举：**必须自己校验**。裸 get<std::string>() 不会拦下 "super_hard"，
      // 而 grading_hints 直接进调色参数，非法值会一路传到下游。
      if (const auto l_v = opt_string(*l_grading, "vignette"); l_v.has_value()) {
        if (is_one_of(*l_v, g_grading_strength_values)) {
          l_spec.targets_.grading_hints_.vignette_ = *l_v;
          put_evidence(l_ret.evidence_, "targets.grading_hints.vignette", evidence_source::prompt, 0.6);
        } else {
          put_evidence(
              l_ret.evidence_, "targets.grading_hints.vignette", evidence_source::default_value, 0.3,
              fmt::format("模型给出的取值 \"{}\" 不在允许集合内，已回退为统计推导值", *l_v)
          );
        }
      }
      if (const auto l_v = opt_bool(*l_grading, "lifted_blacks"); l_v.has_value()) {
        l_spec.targets_.grading_hints_.lifted_blacks_ = *l_v;
        put_evidence(l_ret.evidence_, "targets.grading_hints.lifted_blacks", evidence_source::prompt, 0.6);
      }
      if (const auto l_v = opt_string(*l_grading, "bloom"); l_v.has_value()) {
        if (is_one_of(*l_v, g_grading_strength_values)) {
          l_spec.targets_.grading_hints_.bloom_ = *l_v;
          put_evidence(l_ret.evidence_, "targets.grading_hints.bloom", evidence_source::prompt, 0.6);
        } else {
          put_evidence(
              l_ret.evidence_, "targets.grading_hints.bloom", evidence_source::default_value, 0.3,
              fmt::format("模型给出的取值 \"{}\" 不在允许集合内，已回退为统计推导值", *l_v)
          );
        }
      }
    }
  }

  if (const auto* l_intent_json = opt_object(in_result_json, "intent"); l_intent_json != nullptr) {
    l_spec.intent_.atmosphere_       = opt_string_array(*l_intent_json, "atmosphere");
    l_spec.intent_.time_of_day_      = opt_string(*l_intent_json, "time_of_day").value_or("");
    l_spec.intent_.weather_          = opt_string(*l_intent_json, "weather").value_or("");
    l_spec.intent_.source_motivation_ = opt_string_array(*l_intent_json, "source_motivation");
    l_spec.intent_.story_context_    = opt_string(*l_intent_json, "story_context").value_or("");
    l_spec.intent_.setting_          = opt_string(*l_intent_json, "setting").value_or("");

    if (const auto l_it = l_intent_json->find("character_personality");
        l_it != l_intent_json->end() && l_it->is_array()) {
      for (const auto& l_p : *l_it) {
        if (!l_p.is_object()) continue;
        character_personality_t l_cp{};
        l_cp.subject_ = opt_string(l_p, "subject").value_or("");
        l_cp.trait_   = opt_string(l_p, "trait").value_or("");
        if (const auto* l_ll = opt_object(l_p, "lighting_language"); l_ll != nullptr) {
          if (const auto l_kf = opt_interval(*l_ll, "key_fill_ratio"); l_kf.has_value())
            l_cp.lighting_language_.key_fill_ratio_ = l_kf;
          if (const auto l_v = opt_string(*l_ll, "hardness"); l_v.has_value()) {
            if (is_one_of(*l_v, g_hardness_values))
              l_cp.lighting_language_.hardness_ = *l_v;
            else
              put_evidence(
                  l_ret.evidence_, "intent.character_personality.lighting_language.hardness",
                  evidence_source::default_value, 0.3, fmt::format("非法取值 \"{}\" 已丢弃", *l_v)
              );
          }
          if (const auto l_v = opt_string(*l_ll, "position_bias"); l_v.has_value()) {
            if (is_one_of(*l_v, g_position_bias_values))
              l_cp.lighting_language_.position_bias_ = *l_v;
            else
              put_evidence(
                  l_ret.evidence_, "intent.character_personality.lighting_language.position_bias",
                  evidence_source::default_value, 0.3, fmt::format("非法取值 \"{}\" 已丢弃", *l_v)
              );
          }
          if (const auto l_v = opt_string(*l_ll, "kelvin_bias"); l_v.has_value()) {
            if (is_one_of(*l_v, g_kelvin_bias_values))
              l_cp.lighting_language_.kelvin_bias_ = *l_v;
            else
              put_evidence(
                  l_ret.evidence_, "intent.character_personality.lighting_language.kelvin_bias",
                  evidence_source::default_value, 0.3, fmt::format("非法取值 \"{}\" 已丢弃", *l_v)
              );
          }
        }
        // 主语和性格都空的行没有信息量，丢掉而不是留一堆空壳。
        if (!l_cp.subject_.empty() || !l_cp.trait_.empty())
          l_spec.intent_.character_personality_.emplace_back(std::move(l_cp));
      }
    }

    if (!l_spec.intent_.atmosphere_.empty())
      put_evidence(l_ret.evidence_, "intent.atmosphere", evidence_source::prompt, 0.8);
    if (!l_spec.intent_.character_personality_.empty())
      put_evidence(
          l_ret.evidence_, "intent.character_personality", evidence_source::prompt, 0.7,
          "按 §5.5 的性格→灯光语言映射表落成 lighting_language"
      );
  }

  // ---- 冲突与冲突裁决 ----
  auto l_conflicts = opt_string_array(in_result_json, "conflicts");
  for (auto&& l_conflict : l_conflicts)
    l_ret.reference_stats_.conflicts_.emplace_back(fmt::format("llm: {}", l_conflict));

  l_ret.conflict_resolution_.resolved_["conflicts"]                 = in_aggregated.stats_.conflicts_;
  l_ret.conflict_resolution_.resolved_["per_dimension_confidence"]  = in_aggregated.stats_.per_dimension_confidence_;
  if (!in_aggregated.suspected_outlier_ids_.empty()) {
    // 只报不写：is_outlier_ 是人工排除字段，自动判定写进去会让两种语义在同一列里不可区分（决定 D3）。
    nlohmann::json l_outliers = nlohmann::json::array();
    for (const auto& l_id : in_aggregated.suspected_outlier_ids_) l_outliers.emplace_back(boost::uuids::to_string(l_id));
    l_ret.conflict_resolution_.resolved_["suspected_outliers"] = l_outliers;
  }
  if (!l_overridden.empty()) l_ret.conflict_resolution_.resolved_["overridden_dimensions"] = l_overridden;
  if (!l_conflicts.empty()) l_ret.conflict_resolution_.resolved_["llm_reported"] = l_conflicts;

  std::string l_note = in_aggregated.stats_.conflicts_.empty()
                           ? std::string{"参考图各维度一致：目标取多图中位数，区间取极差。"}
                           : fmt::format(
                                 "以下维度在参考图之间互相矛盾，未设为硬目标，需人工裁决：{}",
                                 join_strings(in_aggregated.stats_.conflicts_, ", ")
                             );
  if (!l_overridden.empty())
    l_note += fmt::format(
        " 提示词与参考不一致、按 §6.3 采用提示词值的维度：{}", join_strings(l_overridden, ", ")
    );
  l_ret.conflict_resolution_.note_ = l_note;

  // 返回值自洽（契约 3）：spec 里带着同一份 evidence，调用方无论读哪一份都一致。
  l_spec.evidence_ = l_ret.evidence_;
  return l_ret;
}

boost::asio::awaitable<spec_generation_result_t> generate_spec(
    sqlite_database& in_sql, const spec_generation_request_t& in_request
) {
  // ---- 1) 统计聚合（纯计算，无 IO；无可用统计也不抛，走 prompt_only 降级）----
  const auto l_agg = aggregate_reference_stats(in_request.references_);

  // ---- 2) 触发人 → AI 工作室（推理密钥的来源）----
  // 用 select + to_optional 而不是 get_by_uuid<T>：后者内部是 to_single()，
  // 「查不到」会变成一条 sqlite 异常（HTTP 500），而这里应当是 400 语义。
  DOODLE_CHICK_HTTP(
      !in_request.operator_id_.is_nil(), bad_request,
      "generate_spec 缺少触发人（operator_id），无法解析 AI 工作室与计费主体"
  );
  const auto l_person =
      select(in_sql).columns(object<person>()).from<person>().where(c(&person::uuid_id_) == in_request.operator_id_)()
          .to_optional();
  DOODLE_CHICK_HTTP(
      l_person.has_value(), bad_request, "找不到触发人 {}（person 不存在），无法解析 AI 工作室",
      in_request.operator_id_
  );
  DOODLE_CHICK_HTTP(
      !l_person->ai_studio_id_.is_nil(), bad_request, "触发人 {} 未关联 AI 工作室，无法调用推理接口",
      in_request.operator_id_
  );
  const auto l_studio = select(in_sql)
                            .columns(object<ai_studio>())
                            .from<ai_studio>()
                            .where(c(&ai_studio::uuid_id_) == l_person->ai_studio_id_)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_studio.has_value(), bad_request, "找不到 AI 工作室 {}（配置可能已被删除）", l_person->ai_studio_id_
  );

  // 每次推理都新建客户端：http_client_ssl 内部只有一个 socket 且无锁（§15.6 第 6 条）。
  auto l_client = http::seedance2::make_inference_client(*l_studio);

  // ---- 3) 组装任务与归一化信封 ----
  seedance2::task l_task{};
  l_task.type_                  = seedance2::task_type::inference;  // not_null，必须显式赋值
  l_task.backend_               = seedance2::task_backend::seedance2;  // 同上
  l_task.user_id_               = in_request.operator_id_;  // 计费主体 + 签名 URL 的 uid
  l_task.ai_studio_id_          = l_studio->uuid_id_;
  l_task.project_uuid_id_       = in_request.project_id_;
  l_task.ai_light_iteration_id_ = uuid{};                   // 蒸馏阶段还没有轮次，留 nil
  l_task.text_prompt_           = in_request.prompt_text_;   // 去重与前台展示都读它
  l_task.completion_tokens_     = doodle_config::g_ark_inference_default_consumed_tokens;
  l_task.data_request_          = build_spec_request(in_request, l_agg);

  // run_inference_task 自己一个事务里落终态任务行 + 扣费（§15.6 的方案 A）。
  // 这是本接缝唯一的副作用，且**有意保留**：推理确实发生了，必须留痕计费 ——
  // 否则调用方回滚会话状态时会连带把 person_token 的扣费抹掉（契约 2 的受控例外）。
  auto l_result = co_await http::seedance2::run_inference_task(l_task, in_sql, l_client);

  // ---- 4) 失败映射：重试全在客户端内做完了（g_ark_inference_max_retry），这里不重试 ----
  if (l_result.status_ != seedance2::task_status::succeeded) {
    const auto l_brief = l_result.result_text_.substr(0, std::min<std::size_t>(200, l_result.result_text_.size()));
    if (http::seedance2::seedance2_client::is_sensitive_content_error(l_result.data_response_)) {
      // 内容违规是可解释的失败：改提示词/换图能解决，所以是 400 而不是 502。
      throw_exception(doodle_error{
          boost::beast::http::status::bad_request, "灯光方案蒸馏被内容安全策略拒绝：{}",
          l_brief.empty() ? l_result.error_code_ : l_brief
      });
    }
    throw_exception(doodle_error{
        boost::beast::http::status::bad_gateway, "推理失败（status={}, error_code={}, 超时={}）：{}",
        magic_enum::enum_name(l_result.status_), l_result.error_code_, l_result.is_timeout_, l_brief
    });
  }

  // result_json_ 由 parse_inference_response 填好（内部已做过 strip_code_fence + parse），
  // **不要**在这里二次解析 result_text_。
  if (!l_result.result_json_.is_object()) {
    const auto l_brief = l_result.result_text_.substr(0, std::min<std::size_t>(200, l_result.result_text_.size()));
    throw_exception(doodle_error{
        boost::beast::http::status::bad_gateway, "模型输出不是合法 JSON 对象，无法映射为 LightingSpec：{}", l_brief
    });
  }

  // ---- 5) 映射 ----
  auto l_ret = map_spec_result(l_result.result_json_, l_agg);
  // 身份字段由调用方（advance_session）负责写进 ai_light_spec，这里只保证自洽。
  l_ret.spec_.session_id_ = in_request.session_id_;

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 蒸馏 Spec 完成：降级 {}，参考图 {} 张（可用统计 {}），token in/out/total={}/{}/{}",
      in_request.session_id_, magic_enum::enum_name(l_ret.spec_.degrade_level_), in_request.references_.size(),
      l_agg.stats_.image_count_, l_result.input_tokens_, l_result.output_tokens_, l_result.total_tokens_
  );

  co_return l_ret;
}

boost::asio::awaitable<candidate_generation_result_t> generate_candidates(
    const candidate_generation_request_t& in_request
) {
  (void)in_request;
  throw_not_implemented("generate_candidates", "接缝本体：轴安排 + 预设库检索 + 调用推理并落候选");
  co_return candidate_generation_result_t{};
}

boost::asio::awaitable<std::vector<axis_step_t>> structure_feedback(
    const uuid& in_session_id, const std::string& in_feedback_text
) {
  (void)in_session_id;
  (void)in_feedback_text;
  throw_not_implemented("structure_feedback", "接缝本体：把自然语言反馈结构化为 axis_step（§13.1 第 9 项）");
  co_return std::vector<axis_step_t>{};
}

}  // namespace doodle::ai_light
