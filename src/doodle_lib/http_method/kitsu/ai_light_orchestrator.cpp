//
// AI 灯光：轮次编排与状态机实现
//
// 对应 docs/ue_ai_lighting_plan.md §10.3（状态流转）与 §10.4（派发契约）。
//
// 三个「接缝」（generate_spec / generate_candidates / structure_feedback）都在本文件末尾：
//   * generate_spec —— 已实现（§15.7）：参考图统计聚合 + 一次火山方舟推理 + 结果映射。
//   * generate_candidates —— 已实现（§15.8）：一次推理给出「基线 rig + 轴计划」，
//     6 个候选由服务端确定性 OFAT 展开（§6.6 的灯集合一致性因此是构造性保证）。
//   * structure_feedback —— 仍抛 501；调用方捕获后降级为纯滑块反馈。
//
// 接缝 2 的分层与接缝 1 相同（纯函数组信封/映射 + 一次网络调用），但多了一层**确定性展开**：
// 模型只决定「往哪个方向搜」，不决定「第 N 张图是什么」。这是有意的 —— 见 build_candidates_input_text
// 上方的注释与 §15.8。
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
#include <functional>
#include <map>
#include <memory>
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
    // 触发人：本次调用的属性，不是会话的属性 —— 所以不进会话表（加列既逼出迁移、
    // 语义也是错的：同一次灯光工作可能换人重出方案）。
    // 没传时回落到会话提交人；两者都没有就没法解析工作室、也没法计费。
    // 值在分支外取（两个接缝都要用它），校验留给各分支给出更贴切的消息。
    const auto l_operator = in_operator_id.is_nil() ? in_session.submitter_ : in_operator_id;

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

      DOODLE_CHICK_HTTP(
          !l_operator.is_nil(), bad_request,
          "会话 {} 无法确定触发人（无 operator_id 且会话无 submitter），无法解析 AI 工作室与计费主体",
          in_session.uuid_id_
      );

      // iteration 的 uuid 在这里就铸造并随请求交给接缝：那条推理任务行（sd2::task）用
      // ai_light_iteration_id_ 指回来，于是「哪一次推理生成了第 N 轮候选」可以反查。
      // 落库时用的是同一个值，所以任务行、迭代行、候选行三者必然一致。
      const auto l_iteration_id = core_set::get_set().get_uuid();

      candidate_generation_request_t l_req{};
      l_req.session_id_   = in_session.uuid_id_;
      l_req.project_id_   = in_session.project_id_;
      l_req.operator_id_  = l_operator;
      l_req.iteration_id_ = l_iteration_id;
      l_req.iteration_    = l_iteration;
      l_req.spec_         = l_spec->to_spec();
      l_req.presets_      = in_sql.get_ai_light_presets(true);
      l_req.prompt_text_  = in_session.prompt_text_;

      // 上一轮的迭代行只查一次，同时喂给下面两件事。
      const auto l_prev_it = in_sql.get_ai_light_iteration(in_session.uuid_id_, in_session.current_iteration_);

      // warm start：上一轮被选中的候选。首轮没有，由接缝从预设库起手。
      //
      // 必须同时要求「上一轮解的 Spec 就是当前这份」：Spec 升版之后（美术重出了方案）
      // 上一轮的 rig 是照着旧目标打的，拿它当起点会让新目标从一开始就被旧方向带偏。
      if (l_prev_it.has_value() && l_prev_it->spec_id_ == l_spec->uuid_id_) {
        if (auto l_base = in_sql.get_ai_light_selected_candidate(in_session.uuid_id_, in_session.current_iteration_);
            l_base.has_value())
          l_req.base_rig_ = l_base->get_rig();
      }

      // 上一轮人工给的轴调整（滑块通道）。首轮没有。
      if (l_prev_it.has_value()) {
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

      // 本轮渲染的基线关卡。**目前为空** —— 取它需要复刻 shot_render_light_builder 的三段
      // 私有推导（基英/集数 → 场景 UE 路径 → 地编预调探测），代价与风险都不合理，
      // 所以本轮只透传、不解析（§15.8 的决定 D8）。空的基线意味着 worker 无法起手渲染，
      // 所以这里明确告警而不是静默放过 —— 这是 M2（工作机执行器）开工前必须补上的一环。
      if (l_req.baseline_level_.empty()) {
        SPDLOG_LOGGER_WARN(
            g_logger_ctrl().get_http(),
            "AI 灯光会话 {} 第 {} 轮：baseline_level 为空（M2 阻塞项，见方案 §15.8 决定 D8）。"
            "候选可以生成，但工作机拿不到起点关卡，无法渲染。",
            in_session.uuid_id_, l_iteration
        );
      }

      auto l_res = co_await generate_candidates(in_sql, l_req);

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
      // 用上面铸造好的那个 uuid，**不要**在这里另取一个：候选的 iteration_id_ 与
      // mark_iteration_rendered 的「候选属于本轮」校验都指向它，而推理任务行
      // （sd2::task.ai_light_iteration_id_）已经用它落了库 —— 再取一个新的就会
      // 让「哪次推理生成本轮」这条链路断开。
      l_it.uuid_id_    = l_iteration_id;
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

std::string build_common_lighting_section() {
  return std::string{R"LIGHT(【职责边界：你决定什么、不决定什么】
你决定：灯的数量、角色、类型、相对方位；环境光 / 雾 / 天空 / 曝光 / 调色的整体倾向；增删改哪些灯（结构决策）。
你不决定：最终精确强度 / 色温 / 距离的数值；直接写 UE API 调用或代码；逐帧关键帧的精确数值；预测渲染结果。
绝对禁止：改变场景几何或材质。你的可改范围只有「灯光 + 环境光/雾/天空 + 后处理/调色」。

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
)LIGHT"};
}

std::string build_spec_instructions() {
  // 共用段（职责边界 + 性格映射表 + 单位口径）只允许有一处定义：两份提示词对「对比度是
  // 什么」说法不一致，模型就会在两条接缝上给出互不相容的数值（见头文件的说明）。
  std::string l_out{
      "你是电影灯光指导。把一个人工提示词与多张参考图的统计，蒸馏成一份可执行的灯光目标（LightingSpec）。\n\n"
  };
  l_out += build_common_lighting_section();
  l_out += R"LIGHT(
【冲突规则：人工提示词优先于参考图】
参考只提供环境与场景的结构与数值，提示词提供意图与语义。
- 提示词与参考一致 → 直接合并，置信度提高。
- 提示词明确、参考冲突 → 提示词胜出。
- 提示词未提及、参考明确 → 参考胜出。
- 提示词未提及、参考冲突 → 该维度不要设为硬目标，把它列进 conflicts，交人工裁决。

【输出要求】
只输出符合给定 JSON Schema 的 JSON。不要输出解释文字，不要用 Markdown 代码块。
调色板（palette）与 ownership 不在 schema 里：颜色只能来自参考图统计，不接受你编造数值。
)LIGHT";
  return l_out;
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

http::seedance2::inference_request_t build_spec_request(
    const spec_generation_request_t& in_request, const aggregated_reference_stats_t& in_aggregated
) {
  // 直接填强类型信封：键名与类型只在 seedance2_inference_request.h 一处定义,
  // 消费端 build_inference_body() 按字段读, 两侧改名/改类型会在编译期暴露。
  http::seedance2::inference_request_t l_req{};
  l_req.instructions_       = build_spec_instructions();
  l_req.input_text_         = build_spec_input_text(in_request, in_aggregated);
  l_req.image_detail_       = "high";
  l_req.temperature_        = 0.2;
  l_req.json_schema_        = build_spec_json_schema();
  l_req.json_schema_name_   = "lighting_spec";
  l_req.json_schema_strict_ = true;
  // 刻意**不设** max_output_tokens：schema 要求全字段 required（含 17 条 evidence），输出长度
  // 不可预估，一旦被截断 result_json_ 就变成不可解析的半个 JSON —— 那个失败面与「平铺形状
  // 被拒」混在一起根本分不出来。上限交给模型/账号侧决定。
  // 不设 thinking：缺省就是 disabled，而结构化任务开 thinking 会引入
  // 「output[0] 是 reasoning 项」那条坑（§15.6 第 2 条）。

  // images 填**站点内相对路径**，由 to_public_url 补公网前缀 + 短时效签名；
  // 绝对磁盘路径（file_path_）绝不能外发。被人工排除的图不发。
  // 空 vector 不会写 images 键（to_json 保证），即「不发 images 空数组」。
  for (const auto& l_ref : in_request.references_) {
    if (l_ref.is_outlier_) continue;
    // reference_image_url 定义在路由头 ai_light.h 的 doodle::http::ai_light 命名空间里。
    l_req.images_.emplace_back(http::ai_light::reference_image_url(l_ref.uuid_id_));
  }
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

// ===========================================================================
// 接缝 2：Spec + 基线 rig + 上一轮反馈轴 → 本轮 6 个候选（方案 §6.2 / §6.6 / §9.1）
// ===========================================================================
//
// 分层与接缝 1 相同（纯函数组信封/映射 + 一次网络调用），但展开器是**确定性**的：
// 模型只输出「一套基线灯集合 + 一份轴计划」，6 个候选由 expand_candidates() 机械展开。
// 于是「同一基线 + 同一轴计划 → 同一组 rig」可以脱网复现（§8.5 第 1 条），
// §6.6 的「同轮候选灯集合与角色完全一致」由构造直接保证，不需要事后校验。
//
// 为什么不给每个候选一次推理：§6.6 要求 6 个候选的灯集合与角色完全相同，只有轴参数不同。
// 让模型分别生成 6 套 rig，就必须事后校验「灯集合是否真的一致」，而模型一旦漂移，本轮
// 要么整体作废要么放弃护栏。分开「决策」（模型：往哪个方向搜）与「展开」（服务端：这 6 张
// 图分别是什么）之后，护栏变成构造性事实。

namespace {

/// 轴计划允许的变异位总数：1 个基线 + 5 个变异位 == g_candidate_count。
constexpr std::size_t g_axis_step_budget = static_cast<std::size_t>(g_candidate_count) - 1;

/// 每轮最多声明几个轴。太多轴会让「差别从哪来」变得看不出来，而美术的判断正是收敛信号。
constexpr std::size_t g_axis_max_count = 2;

/// axes_.strategy_ 是 TEXT 列，模型可以长篇大论；截断避免一次输出把这一列撑爆。
constexpr std::size_t g_strategy_max_len = 500;

/// 提示词里最多列几个预设。预设库首批只有几十条，全在提示词里列一遍没有必要。
constexpr std::size_t g_preset_prompt_top_k = 12;

/// 轴值语义（§15.8 的表）。提示词与 apply_axis_step 共用同一份口径：
/// 模型按它给档位，服务端按同一套公式应用 —— 两边不一致时美术会看到「轴变了但图没变」。
///
/// **只放语义，不放约束**（§15.9 决定）：接缝 2 的「恰好 5 个档位 / 档位不要取 0」是排轴
/// 环节的硬约束，接缝 3 只给**一个**方向值，两者约束不同。把它们混在这一段里，
/// 接缝 3 的提示词会连带继承一套不适用的档位规则。
constexpr std::string_view g_axis_value_semantics_text = R"AXIS(每个轴的 value 都是**增量**（不是绝对值），中性值是 0；只有 key_fill_ratio 例外，它本身就是目标比值。
- key_direction     方位角增量（度）。正负决定主光往哪边偏；常见档位 -60 / -30 / +30 / +60。
- key_fill_ratio    目标 key:fill 比值（绝对，如 3 / 6 / 10）。服务端按「当前比值 ÷ 目标比值」缩放补光强度。
- kelvin            色温增量（K）。同时作用于灯光色温与全局白平衡；常见档位 -800 / -400 / +400 / +800。
- hardness          硬软增量（-1..+1，正 = 更硬）。同时收紧影边缘与缩小光源半径；常见档位 -0.5 / +0.5。
- ambient_level     环境光相对增减（-1..+1）。作用于天光强度与 role=ambient 的灯；常见档位 -0.4 / +0.4。
- grading_strength  调色强度增量（-1..+1），只放大/缩小「偏离中性的量」（对比、饱和、提黑、暗角、光晕）；
                    常见档位 -0.35 / +0.35。
)AXIS";

/// 可选的搜索轴：variant_axis_kind 去掉 none（none 是「这是基线」的标记，不是可搜索的自由度）。
nlohmann::json make_axis_enum_schema() {
  nlohmann::json l_enum = nlohmann::json::array();
  const auto l_none     = magic_enum::enum_name(variant_axis_kind::none);
  for (auto&& l_v : magic_enum::enum_names<variant_axis_kind>()) {
    if (l_v == l_none) continue;
    l_enum.emplace_back(l_v);
  }
  return nlohmann::json{{"type", "string"}, {"enum", l_enum}};
}

std::string join_numbers(const std::vector<double>& in_values) {
  std::string l_ret{};
  for (std::size_t l_i = 0; l_i < in_values.size(); ++l_i) {
    if (l_i != 0) l_ret += ", ";
    l_ret += fmt::format("{}", in_values[l_i]);
  }
  return l_ret;
}

// ---- 严格取值器 ----------------------------------------------------------
// 基线 rig 的物化是「模型给什么就用什么」的那一段，所以这里**不用** issue 1 的
// opt_xxx（那套是「能读就读、读不出用默认」）。strict schema 下缺字段本该不可能发生；
// 发生了说明模型没遵守契约，静默取默认值会把「模型没按要求输出」变成一个看不出来的偏移。

double require_number(const nlohmann::json& in_json, const char* in_key) {
  DOODLE_CHICK_HTTP(
      in_json.is_object() && in_json.contains(in_key) && in_json.at(in_key).is_number(), bad_gateway,
      "候选基线 rig 的字段 {} 缺失或不是数字", in_key
  );
  return in_json.at(in_key).get<double>();
}

std::string require_string(const nlohmann::json& in_json, const char* in_key) {
  DOODLE_CHICK_HTTP(
      in_json.is_object() && in_json.contains(in_key) && in_json.at(in_key).is_string(), bad_gateway,
      "候选基线 rig 的字段 {} 缺失或不是字符串", in_key
  );
  return in_json.at(in_key).get<std::string>();
}

bool require_bool(const nlohmann::json& in_json, const char* in_key) {
  DOODLE_CHICK_HTTP(
      in_json.is_object() && in_json.contains(in_key) && in_json.at(in_key).is_boolean(), bad_gateway,
      "候选基线 rig 的字段 {} 缺失或不是布尔值", in_key
  );
  return in_json.at(in_key).get<bool>();
}

nlohmann::json require_number_array3(const nlohmann::json& in_json, const char* in_key) {
  const bool l_ok = in_json.is_object() && in_json.contains(in_key) && in_json.at(in_key).is_array() &&
                    in_json.at(in_key).size() == 3 && in_json.at(in_key).at(0).is_number() &&
                    in_json.at(in_key).at(1).is_number() && in_json.at(in_key).at(2).is_number();
  DOODLE_CHICK_HTTP(l_ok, bad_gateway, "候选基线 rig 的字段 {} 必须是 3 个数字的数组", in_key);
  return in_json.at(in_key);
}

/// 枚举取值一律走 magic_enum::enum_cast 显式校验。**绝不能**用 NLOHMANN_JSON_SERIALIZE_ENUM
/// 的裸 get<>：它遇到认不出的字符串**不抛**，而是静默落到第一个枚举值
/// （nlohmann/detail/macro_scope.hpp:247）—— 那等于把「模型输出了一个非法灯角色」变成
/// 一盏看起来正常的 key 灯，整轮候选都建立在一个错值上。
template <typename E>
E require_enum(const nlohmann::json& in_json, const char* in_key) {
  const auto l_name = require_string(in_json, in_key);
  const auto l_v    = magic_enum::enum_cast<E>(l_name);
  DOODLE_CHICK_HTTP(
      l_v.has_value(), bad_gateway, "候选基线 rig 的字段 {} = \"{}\" 不是合法的枚举取值", in_key, l_name
  );
  return *l_v;
}

// ---- rig 取值助手 --------------------------------------------------------

light_t* first_light_with_role(lighting_rig_t& io_rig, light_role in_role) {
  for (auto& l_light : io_rig.lights_)
    if (l_light.role_ == in_role) return &l_light;
  return nullptr;
}

/// 只比较「灯怎么打」的部分：身份字段（rig_id/spec_id/iteration/candidate_index）、
/// variant_axis_、provenance_、ops_ 都由服务端另填，拿它们判「候选是否与基线相同」会误判。
nlohmann::json rig_substance(const lighting_rig_t& in_rig) {
  return nlohmann::json{{"globals", in_rig.globals_}, {"lights", in_rig.lights_}};
}

std::string describe_rig_brief(const lighting_rig_t& in_rig) {
  std::string l_out = fmt::format(
      "  globals: exposure_bias={:.2f} white_balance_kelvin={:.0f} sky_light_intensity={:.2f} fog_density={:.2f}\n"
      "           grading(contrast={:.2f} saturation={:.2f} lift={:.2f} vignette={:.2f} bloom={:.2f})\n",
      in_rig.globals_.exposure_.ev100_bias_, in_rig.globals_.white_balance_.kelvin_,
      in_rig.globals_.environment_.sky_light_intensity_, in_rig.globals_.environment_.fog_density_,
      in_rig.globals_.grading_.contrast_, in_rig.globals_.grading_.saturation_, in_rig.globals_.grading_.lift_,
      in_rig.globals_.grading_.vignette_, in_rig.globals_.grading_.bloom_
  );
  l_out += fmt::format("  灯 {} 个：\n", in_rig.lights_.size());
  for (const auto& l_light : in_rig.lights_) {
    l_out += fmt::format(
        "    role={} type={} azimuth={:.1f} elevation={:.1f} distance={:.2f} intensity={:.1f}({}) "
        "kelvin={:.0f} softness={:.2f} radius_cm={:.1f} cast_shadows={}\n",
        magic_enum::enum_name(l_light.role_), magic_enum::enum_name(l_light.type_), l_light.offset_.azimuth_deg_,
        l_light.offset_.elevation_deg_, l_light.offset_.distance_m_, l_light.intensity_.value_,
        magic_enum::enum_name(l_light.intensity_.unit_), l_light.color_.kelvin_, l_light.shadows_.softness_,
        l_light.shape_.source_radius_cm_, l_light.shadows_.cast_ ? "true" : "false"
    );
  }
  return l_out;
}

/// Spec 的紧凑投影。这里是给模型读的，不是给落库用的 —— 落库的完整形状在 ai_light_spec。
std::string describe_spec_brief(const lighting_spec_t& in_spec) {
  std::string l_out{};
  l_out += fmt::format("  氛围: {}\n", join_strings(in_spec.intent_.atmosphere_, "、"));
  l_out += fmt::format("  时间/天气: {} / {}\n", in_spec.intent_.time_of_day_, in_spec.intent_.weather_);
  l_out += fmt::format("  场景设定: {}\n", in_spec.intent_.setting_);
  if (!in_spec.intent_.source_motivation_.empty())
    l_out += fmt::format("  光源动机: {}\n", join_strings(in_spec.intent_.source_motivation_, "、"));
  if (!in_spec.intent_.story_context_.empty())
    l_out += fmt::format("  故事上下文: {}\n", in_spec.intent_.story_context_);
  for (const auto& l_person : in_spec.intent_.character_personality_) {
    const auto& l_lang = l_person.lighting_language_;
    const std::string l_ratio =
        l_lang.key_fill_ratio_.has_value()
            ? fmt::format("[{}, {}]", l_lang.key_fill_ratio_->low_, l_lang.key_fill_ratio_->high_)
            : std::string{"未指定"};
    l_out += fmt::format(
        "  角色「{}」({}): key_fill_ratio={} hardness={} position_bias={} kelvin_bias={}\n", l_person.subject_,
        l_person.trait_, l_ratio, l_lang.hardness_, l_lang.position_bias_, l_lang.kelvin_bias_
    );
  }
  l_out += "  目标：\n";
  l_out += describe_targets(in_spec.targets_);
  if (!in_spec.targets_.palette_.empty()) {
    l_out += fmt::format("  palette {} 色（颜色只能来自参考图统计）：", in_spec.targets_.palette_.size());
    for (const auto& l_entry : in_spec.targets_.palette_)
      l_out += fmt::format(
          "{}[{:.2f},{:.2f}] ", rgb_to_hex(l_entry.color_), l_entry.share_.low_, l_entry.share_.high_
      );
    l_out += "\n";
  }
  if (!in_spec.reference_stats_.conflicts_.empty())
    l_out += fmt::format(
        "  参考图冲突维度（不要把它们当硬目标）：{}\n", join_strings(in_spec.reference_stats_.conflicts_, ", ")
    );
  return l_out;
}

/// 预设检索走词法，不走向量：全仓库**没有**可用于「按 Spec 检索预设」的查询向量生成器
/// （唯一的 embedding 生成器是 ONNX 的 ai/llm2vec，被 kimodo 用，与预设的 CLIP 图编码
/// 不是一回事）。中文没有词边界，所以用子串包含而不是分词 ——「霓虹」⊂「夜景霓虹」正是要的。
std::size_t preset_lexical_score(const ai_light_preset& in_preset, const std::string& in_haystack) {
  std::size_t l_score = 0;
  if (!in_preset.name_.empty() && in_haystack.find(in_preset.name_) != std::string::npos) l_score += 4;
  for (const auto& l_tag : in_preset.tags_)
    if (!l_tag.empty() && in_haystack.find(l_tag) != std::string::npos) l_score += 2;
  return l_score;
}

/// 同分按名字排序，保证同一份 Spec 得到同一份提示词（§8.5 第 1 条：可复现）。
std::vector<const ai_light_preset*> pick_presets(
    const candidate_generation_request_t& in_request, std::size_t in_top_k
) {
  std::string l_haystack = in_request.spec_.intent_.time_of_day_ + in_request.spec_.intent_.weather_ +
                           in_request.spec_.intent_.setting_ + in_request.spec_.intent_.story_context_;
  for (const auto& l_v : in_request.spec_.intent_.atmosphere_) l_haystack += l_v;
  for (const auto& l_v : in_request.spec_.intent_.source_motivation_) l_haystack += l_v;
  for (const auto& l_v : in_request.spec_.intent_.character_personality_) l_haystack += l_v.trait_ + l_v.subject_;

  std::vector<const ai_light_preset*> l_cands{};
  l_cands.reserve(in_request.presets_.size());
  for (const auto& l_preset : in_request.presets_)
    if (l_preset.enabled_) l_cands.emplace_back(&l_preset);

  std::sort(l_cands.begin(), l_cands.end(), [&l_haystack](const ai_light_preset* in_l, const ai_light_preset* in_r) {
    const auto l_sl = preset_lexical_score(*in_l, l_haystack);
    const auto l_sr = preset_lexical_score(*in_r, l_haystack);
    if (l_sl != l_sr) return l_sl > l_sr;
    return in_l->name_ < in_r->name_;
  });
  if (l_cands.size() > in_top_k) l_cands.resize(in_top_k);
  return l_cands;
}

std::string describe_preset_brief(const ai_light_preset& in_preset) {
  const auto l_target = in_preset.get_stats_target();
  return fmt::format(
      "  - {}  tags=[{}]  stats_target(contrast=[{}, {}] kelvin=[{}, {}])\n", in_preset.name_,
      join_strings(in_preset.tags_, ", "), l_target.contrast_.low_, l_target.contrast_.high_,
      l_target.white_balance_kelvin_.low_, l_target.white_balance_kelvin_.high_
  );
}

/// 解析并校验模型的轴计划。返回失败原因（nullopt = 合法）。
///
/// 校验项都针对「静默失败会变成 6 张看不出区别的图」这一类：轴名是否合法、是否重复、
/// 档位总数够不够、有没有中性档。**不合法不抛错** —— 轴计划是模型的一种建议，弃用它
/// 退回默认计划仍然能给出 6 个可用候选，比让整轮失败好。
std::optional<std::string> parse_axis_plan(
    const nlohmann::json& in_json, iteration_axes_t& out_axes, std::string& out_strategy
) {
  out_strategy = opt_string(in_json, "strategy").value_or(std::string{});
  if (out_strategy.size() > g_strategy_max_len) out_strategy.resize(g_strategy_max_len);

  if (!in_json.is_object() || !in_json.contains("axes") || !in_json.at("axes").is_array())
    return std::string{"模型没有给出 axes 数组"};
  const auto& l_axes_json = in_json.at("axes");
  if (l_axes_json.empty() || l_axes_json.size() > g_axis_max_count)
    return fmt::format("axes 有 {} 个（只允许 1..{} 个）", l_axes_json.size(), g_axis_max_count);

  std::vector<axis_step_t> l_steps{};
  std::size_t l_total = 0;
  for (const auto& l_item : l_axes_json) {
    if (!l_item.is_object() || !l_item.contains("axis") || !l_item.at("axis").is_string())
      return std::string{"axes 的元素缺少 axis"};
    const auto l_kind = magic_enum::enum_cast<variant_axis_kind>(l_item.at("axis").get<std::string>());
    if (!l_kind.has_value()) return fmt::format("轴名 \"{}\" 不是合法的轴", l_item.at("axis").get<std::string>());
    if (*l_kind == variant_axis_kind::none) return std::string{"none 不是可搜索的轴"};
    for (const auto& l_prev : l_steps)
      if (l_prev.axis_ == *l_kind) return fmt::format("轴 {} 出现了两次", magic_enum::enum_name(*l_kind));

    if (!l_item.contains("steps") || !l_item.at("steps").is_array() || l_item.at("steps").empty())
      return fmt::format("轴 {} 没有 steps", magic_enum::enum_name(*l_kind));

    axis_step_t l_step{};
    l_step.axis_  = *l_kind;
    l_step.label_ = opt_string(l_item, "label").value_or(std::string{});
    if (l_step.label_.empty()) l_step.label_ = std::string{magic_enum::enum_name(*l_kind)};
    for (const auto& l_v : l_item.at("steps")) {
      if (!l_v.is_number()) return fmt::format("轴 {} 的档位里有一个不是数字", magic_enum::enum_name(*l_kind));
      const double l_value = l_v.get<double>();
      // key_direction / kelvin / hardness / ambient_level / grading_strength 的中性值都是 0；
      // key_fill_ratio 是比值，本身不存在 0 档。放行 0 档会当场产生一个与候选 0 相同的候选，
      // 服务端的同一个候选检查会在后面判失败 —— 不如在这里给出可读的原因。
      if (l_value == 0.0)
        return fmt::format("轴 {} 的档位包含 0（等价于基线，会得到一个重复候选）", magic_enum::enum_name(*l_kind));
      l_step.steps_.emplace_back(l_value);
    }
    l_total += l_step.steps_.size();
    l_steps.emplace_back(std::move(l_step));
  }
  if (l_total != g_axis_step_budget)
    return fmt::format(
        "档位总数是 {}，必须是 {}（1 个基线 + {} 个变异 = {} 个候选）", l_total, g_axis_step_budget,
        g_axis_step_budget, g_candidate_count
    );

  out_axes.axes_ = std::move(l_steps);
  return std::nullopt;
}

}  // namespace

std::string build_candidates_instructions() {
  std::string l_out{"你是电影灯光指导的**执行**环节。已经有一份人工确认过的灯光方案（LightingSpec），"
                    "现在要把它变成一组可以真的渲出来的灯光设置，并给出下一轮要试的方向。\n\n"};
  l_out += build_common_lighting_section();
  l_out += "\n【你要输出两样东西】\n"
           "1) 一套**基线灯集合**（globals + lights）：这是本轮 6 个候选共同的起点。\n"
           "2) 一份**轴计划**（axes）：在基线上沿哪几个方向、各试哪几档，共 5 个档位。\n"
           "6 个候选 = 1 个基线 + 5 个变异，由服务端按你的轴计划机械展开 —— 你**只给一套灯**，\n"
           "不要输出 6 套 rig。这样 6 张图的灯集合与角色必然一致（方案 §6.6 的护栏）。\n\n"
           "【轴值语义】\n";
  l_out += g_axis_value_semantics_text;
  // 排轴环节专属的硬约束。放在这里而不是语义段里：接缝 3 复用同一份语义段，
  // 但它的产出是「每条轴一个方向值」，套用这套档位规则会自相矛盾（§15.9 决定）。
  l_out += "\n硬约束：所有轴加起来必须有恰好 5 个档位（1 个基线 + 5 个变异 = 6 个候选）。\n"
           "档位不要取 0：0 档等价于基线，会得到一个与基线一模一样的候选，服务端会因此判本轮失败。\n"
           "最多声明 2 个轴（每轮只动少数几个自由度，美术才看得出差别从哪来）。\n";
  l_out += "\n【第一轮与后续轮次】\n"
           "首轮（没有上一轮 rig）：从 Spec 与预设出发搭一套完整灯集合，档位取**大跨度**，\n"
           "目的是先把搜索空间铺开，让美术一眼看出各方向差在哪。\n"
           "后续轮：以上一轮美术选中的 rig 为起点，**只在需要改的地方改**，档位取**小跨度**，\n"
           "在美术选中的区域附近收缩细化（§9.1：把人工判断从 yes/no 变成有效的搜索梯度）。\n\n"
           "【约束】\n"
           "- 至少要有 role=key 的主光：key_direction 与 key_fill_ratio 两个轴都作用在它身上，\n"
           "  没有主光这两个轴就是空操作。\n"
           "- 灯的角色与数量由你定，但同一轮内 6 个候选共用同一套灯（服务端保证）。\n"
           "- 曝光在迭代期必须手动锁定（method 固定 manual、ev100_bias 人为给定），\n"
           "  否则自动曝光会把强度改动吃掉，美术看到的差别来自测光而不是布光。\n"
           "- 环境光也算灯：天光强度、雾密度在 globals 里给。\n\n"
           "【输出要求】\n"
           "只输出符合给定 JSON Schema 的 JSON。不要输出解释文字，不要用 Markdown 代码块。\n"
           "color_mode 决定 color 的哪个分支生效：temperature 时填 kelvin，rgb 时填 rgb 三元组，\n"
           "filter 时填 filter 名字；另外两个分支按 schema 给中性占位（rgb 填 [0,0,0]、filter 填空串）。\n";
  return l_out;
}

nlohmann::json build_candidates_json_schema() {
  nlohmann::json l_axis_item{};
  l_axis_item["type"]                 = "object";
  l_axis_item["additionalProperties"] = false;
  l_axis_item["properties"]           = nlohmann::json{
      {"axis", make_axis_enum_schema()},
      {"steps",
       nlohmann::json{
           {"type", "array"}, {"items", {{"type", "number"}}}, {"minItems", 1}, {"maxItems", g_axis_step_budget}
       }},
      {"label", {{"type", "string"}}},
  };
  l_axis_item["required"] = nlohmann::json::array({"axis", "steps", "label"});

  nlohmann::json l_globals{};
  l_globals["type"]                 = "object";
  l_globals["additionalProperties"] = false;
  l_globals["properties"]           = nlohmann::json{
      {"exposure_ev100_bias", {{"type", "number"}}},
      {"white_balance_kelvin", {{"type", "number"}, {"minimum", 1500.0}, {"maximum", 15000.0}}},
      {"sky_light_intensity", {{"type", "number"}, {"minimum", 0.0}}},
      {"fog_density", {{"type", "number"}, {"minimum", 0.0}}},
      {"grading_contrast", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 3.0}}},
      {"grading_saturation", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 3.0}}},
      {"grading_lift", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 1.0}}},
      {"grading_vignette", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 1.0}}},
      {"grading_bloom", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 1.0}}},
  };
  l_globals["required"] = nlohmann::json::array(
      {"exposure_ev100_bias", "white_balance_kelvin", "sky_light_intensity", "fog_density", "grading_contrast",
       "grading_saturation", "grading_lift", "grading_vignette", "grading_bloom"}
  );

  nlohmann::json l_light{};
  l_light["type"]                 = "object";
  l_light["additionalProperties"] = false;
  l_light["properties"]           = nlohmann::json{
      {"role", make_enum_schema<light_role>()},
      {"type", make_enum_schema<light_type>()},
      {"azimuth_deg", {{"type", "number"}, {"minimum", -180.0}, {"maximum", 180.0}}},
      {"elevation_deg", {{"type", "number"}, {"minimum", -90.0}, {"maximum", 90.0}}},
      {"distance_m", {{"type", "number"}, {"minimum", 0.1}}},
      {"intensity_unit", make_enum_schema<intensity_unit>()},
      {"intensity", {{"type", "number"}, {"minimum", 0.0}}},
      {"color_mode", make_enum_schema<color_mode>()},
      {"kelvin", {{"type", "number"}, {"minimum", 1500.0}, {"maximum", 15000.0}}},
      {"rgb",
       nlohmann::json{
           {"type", "array"},
           {"items", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 1.0}}},
           {"minItems", 3},
           {"maxItems", 3}
       }},
      {"filter", {{"type", "string"}}},
      {"softness", {{"type", "number"}, {"minimum", 0.0}, {"maximum", 1.0}}},
      {"source_radius_cm", {{"type", "number"}, {"minimum", 0.0}}},
      {"cast_shadows", {{"type", "boolean"}}},
      {"note", {{"type", "string"}}},
  };
  // 全字段必填：颜色三个分支只能有一个生效，但严格结构化输出不支持「条件必填」，
  // 所以三个分支的字段都必填，非本分支的填中性占位（提示词里已说明）。
  l_light["required"] = nlohmann::json::array(
      {"role", "type", "azimuth_deg", "elevation_deg", "distance_m", "intensity_unit", "intensity", "color_mode",
       "kelvin", "rgb", "filter", "softness", "source_radius_cm", "cast_shadows", "note"}
  );

  // 刻意没有：variant_axis / baseline_level / ops / rig_id / candidate_index / schema_version ——
  // 这些是服务端的字段（谁在哪一轮、哪个候选、从哪个关卡起手），模型给它们没有意义，
  // 而一旦进了 schema，模型就会开始编造它们。
  nlohmann::json l_root{};
  l_root["type"]                 = "object";
  l_root["additionalProperties"] = false;
  l_root["properties"]           = nlohmann::json{
      {"strategy", {{"type", "string"}}},
      {"axes",
       nlohmann::json{
           {"type", "array"}, {"items", l_axis_item}, {"minItems", 1}, {"maxItems", g_axis_max_count}
       }},
      {"globals", l_globals},
      // 不设 maxItems：决策 17 明确「单镜头动态阴影灯上限先不限制」，这里加一个上限
      // 等于在 schema 层偷偷引入一条新的性能约束。
      {"lights", nlohmann::json{{"type", "array"}, {"items", l_light}, {"minItems", 1}}},
  };
  l_root["required"] = nlohmann::json::array({"strategy", "axes", "globals", "lights"});
  return l_root;
}

std::string build_candidates_input_text(const candidate_generation_request_t& in_request) {
  std::string l_out{};
  l_out += fmt::format(
      "【已确认的灯光方案（LightingSpec，人工已确认）】降级级别={}\n",
      magic_enum::enum_name(in_request.spec_.degrade_level_)
  );
  l_out += describe_spec_brief(in_request.spec_);

  const auto l_presets = pick_presets(in_request, g_preset_prompt_top_k);
  if (l_presets.empty()) {
    l_out += "\n【预设库】空的（或都被停用了）：请完全依据上面的方案自己给出一套基线灯集合。\n";
  } else {
    l_out += fmt::format("\n【可选预设（按标签匹配挑出的 {} 条，可以只借鉴其中一部分）】\n", l_presets.size());
    for (const auto* l_preset : l_presets) l_out += describe_preset_brief(*l_preset);
  }

  if (in_request.base_rig_.has_value()) {
    l_out += fmt::format(
        "\n【上一轮美术选中的 rig（第 {} 轮的起点，warm start）】\n", in_request.iteration_ - 1
    );
    l_out += describe_rig_brief(*in_request.base_rig_);
    l_out += "除非下面的 Spec 或反馈明确要求，**保持灯的角色与数量不变**，只调整数值与方位。\n";
  } else {
    l_out += "\n【首轮：没有上一轮的 rig】\n"
             "请从上面的方案与预设出发给出一套完整的基线灯集合；首轮档位取大跨度。\n";
  }

  if (in_request.feedback_axes_.has_value() && !in_request.feedback_axes_->axes_.empty()) {
    l_out += "\n【美术用滑块指定的方向（这些轴必须出现在你的 axes 里）】\n";
    for (const auto& l_axis : in_request.feedback_axes_->axes_)
      l_out += fmt::format("  {} = {}\n", magic_enum::enum_name(l_axis.axis_), join_numbers(l_axis.steps_));
    l_out += "美术已经把方向指出来了：请让对应轴的档位覆盖（或落在）这个方向附近，"
             "并把档位总数补齐到 5。\n";
  } else if (in_request.iteration_ > 1) {
    l_out += "\n【本轮没有滑块指定的方向】\n"
             "美术只是要求继续迭代；请根据上一轮选中的 rig 与目标之间的差距自己选轴，档位取小跨度。\n";
  }
  return l_out;
}

http::seedance2::inference_request_t build_candidates_request(const candidate_generation_request_t& in_request) {
  http::seedance2::inference_request_t l_req{};
  l_req.instructions_       = build_candidates_instructions();
  l_req.input_text_         = build_candidates_input_text(in_request);
  l_req.temperature_        = 0.2;
  l_req.json_schema_        = build_candidates_json_schema();
  l_req.json_schema_name_   = "lighting_candidates";
  l_req.json_schema_strict_ = true;
  // 刻意**不发**的键，每一个都有理由：
  //   images / image_detail  —— Spec 已经把参考蒸馏成数字目标了，生成阶段不需要再看图；
  //                             没有 images 还发 image_detail 就是给「不存在的输入」配参数。
  //   max_output_tokens      —— 缺省值 4096 会截断较大的输出，而截断的表现（result_json_ 变
  //                             null）与「schema 被拒」无法区分，排查时会走错方向（§15.6 坑 21）。
  //   thinking               —— 缺省 disabled；结构化任务开 thinking 会让 output[0] 变成
  //                             reasoning（§15.7）。
  return l_req;
}

iteration_axes_t default_axis_plan(const lighting_spec_t& in_spec) {
  // 档位直接取自 §6.2 自己给出的数字（3:1 / 6:1 / 10:1 与冷暖），是「模型没能给出可用搜索
  // 方向」时唯一有依据的起点。刻意不含中性档：中性档等于基线，会被同一个候选检查拦下。
  iteration_axes_t l_axes{};
  axis_step_t l_ratio{};
  l_ratio.axis_  = variant_axis_kind::key_fill_ratio;
  l_ratio.steps_ = {3.0, 6.0, 10.0};
  l_ratio.label_ = "对比档位";
  axis_step_t l_kelvin{};
  l_kelvin.axis_  = variant_axis_kind::kelvin;
  l_kelvin.steps_ = {-800.0, 800.0};
  l_kelvin.label_ = "冷暖";
  l_axes.axes_    = {l_ratio, l_kelvin};
  l_axes.strategy_ = fmt::format(
      "默认轴计划：先用 key:fill 的 {} 三档看对比结构，再用色温 {} 两档看冷暖。", join_numbers(l_ratio.steps_),
      join_numbers(l_kelvin.steps_)
  );
  if (!in_spec.reference_stats_.conflicts_.empty())
    l_axes.strategy_ += fmt::format(
        "（参考图存在冲突维度，未设为硬目标：{}）", join_strings(in_spec.reference_stats_.conflicts_, ", ")
    );
  return l_axes;
}

lighting_rig_t build_base_rig(const nlohmann::json& in_json) {
  DOODLE_CHICK_HTTP(in_json.is_object(), bad_gateway, "候选基线 rig 的输出不是 JSON 对象");
  DOODLE_CHICK_HTTP(
      in_json.contains("globals") && in_json.at("globals").is_object(), bad_gateway, "候选基线 rig 缺少 globals 对象"
  );
  DOODLE_CHICK_HTTP(
      in_json.contains("lights") && in_json.at("lights").is_array() && !in_json.at("lights").empty(), bad_gateway,
      "候选基线 rig 缺少 lights 数组（至少要有一盏灯）"
  );
  const auto& l_globals = in_json.at("globals");
  const auto& l_lights  = in_json.at("lights");

  lighting_rig_t l_rig{};
  l_rig.schema_version_ = std::string{g_rig_schema_version};

  l_rig.globals_.exposure_.ev100_bias_             = require_number(l_globals, "exposure_ev100_bias");
  l_rig.globals_.exposure_.method_                 = "manual";  // §7.5：迭代期曝光必须手动锁定
  l_rig.globals_.white_balance_.kelvin_            = require_number(l_globals, "white_balance_kelvin");
  l_rig.globals_.white_balance_.tint_              = 0.0;       // 色偏暂不进 schema（见 §15.8 待实测）
  l_rig.globals_.environment_.sky_light_intensity_ = require_number(l_globals, "sky_light_intensity");
  l_rig.globals_.environment_.fog_density_         = require_number(l_globals, "fog_density");
  l_rig.globals_.grading_.contrast_                = require_number(l_globals, "grading_contrast");
  l_rig.globals_.grading_.saturation_              = require_number(l_globals, "grading_saturation");
  l_rig.globals_.grading_.lift_                    = require_number(l_globals, "grading_lift");
  l_rig.globals_.grading_.vignette_                = require_number(l_globals, "grading_vignette");
  l_rig.globals_.grading_.bloom_                   = require_number(l_globals, "grading_bloom");

  std::int32_t l_index = 0;
  for (const auto& l_json : l_lights) {
    DOODLE_CHICK_HTTP(l_json.is_object(), bad_gateway, "候选基线 rig 的 lights[{}] 不是对象", l_index);
    light_t l_light{};
    // id 由服务端生成：模型给的 id 可能与它自己的其它灯重名，而 id 是 ops_ 的对齐键，
    // 重名会让 diff 与执行器都指向错误的灯。
    l_light.id_                      = fmt::format("L{:02}", l_index++);
    l_light.role_                    = require_enum<light_role>(l_json, "role");
    l_light.type_                    = require_enum<light_type>(l_json, "type");
    l_light.offset_.azimuth_deg_     = require_number(l_json, "azimuth_deg");
    l_light.offset_.elevation_deg_   = require_number(l_json, "elevation_deg");
    l_light.offset_.distance_m_      = require_number(l_json, "distance_m");
    l_light.intensity_.unit_         = require_enum<intensity_unit>(l_json, "intensity_unit");
    l_light.intensity_.value_        = require_number(l_json, "intensity");
    l_light.shadows_.softness_       = std::clamp(require_number(l_json, "softness"), 0.0, 1.0);
    l_light.shadows_.cast_           = require_bool(l_json, "cast_shadows");
    l_light.shape_.source_radius_cm_ = std::max(0.0, require_number(l_json, "source_radius_cm"));
    l_light.color_.mode_             = require_enum<color_mode>(l_json, "color_mode");
    // rgb 在 schema 里是必填三元组，**形状与 color_mode 无关地校验**：长度不对就说明模型没有按
    // schema 回答。若因为「当前模式用不到它」而静默忽略，一次明确的协议违规就变成了一盏看起来
    // 完全正常的灯 —— 与裸 `get<Enum>()` 静默落值属于同一类问题。
    if (const auto l_rgb_it = l_json.find("rgb"); l_rgb_it != l_json.end() && !l_rgb_it->is_null())
      require_number_array3(l_json, "rgb");
    switch (l_light.color_.mode_) {
      case color_mode::temperature:
        l_light.color_.kelvin_ = require_number(l_json, "kelvin");
        break;
      case color_mode::rgb: {
        const auto l_rgb    = require_number_array3(l_json, "rgb");
        l_light.color_.rgb_ = vec3_t{l_rgb.at(0).get<double>(), l_rgb.at(1).get<double>(), l_rgb.at(2).get<double>()};
        break;
      }
      case color_mode::filter:
        l_light.color_.filter_ = require_string(l_json, "filter");
        break;
      default:
        break;
    }
    // 其余 IR 字段走结构体默认值（anchor 默认相机空间、channels 默认 [0,1,0] 主光走 Channel1、
    // mobility=movable、origin=ai、cone 角留给执行器按灯型补）—— 这些是 IR 细节，
    // 让模型填只会多一处可以填错的地方。
    l_rig.lights_.emplace_back(std::move(l_light));
  }
  return l_rig;
}

/// 当前 key:fill 比。除了 apply_axis_step，接缝 3 的提示词也要用它 —— key_fill_ratio 是
/// **绝对值**轴，模型必须知道现状才能给出新值（「对比再强一点」不是一个可直接使用的数字）。
///
/// 与 apply_axis_step 的判定条件**逐条一致**：key 存在且强度 > 0、所有 fill 灯与 key
/// **同 intensity_unit_**（坎德拉与勒克斯不是同一个量纲，比值没有意义）、fill 强度和 > 0。
std::optional<double> key_fill_ratio_of(const lighting_rig_t& in_rig) {
  // 这里不调 first_light_with_role：它取的是可写引用（apply_axis_step 要改灯），
  // 而本函数是只读的。
  const light_t* l_key = nullptr;
  for (const auto& l_light : in_rig.lights_)
    if (l_light.role_ == light_role::key) {
      l_key = &l_light;
      break;
    }
  if (l_key == nullptr) return std::nullopt;
  if (l_key->intensity_.value_ <= 0.0) return std::nullopt;
  double l_fill_sum = 0.0;
  for (const auto& l_light : in_rig.lights_) {
    if (l_light.role_ != light_role::fill) continue;
    if (l_light.intensity_.unit_ != l_key->intensity_.unit_) return std::nullopt;
    l_fill_sum += l_light.intensity_.value_;
  }
  if (l_fill_sum <= 0.0) return std::nullopt;
  return l_key->intensity_.value_ / l_fill_sum;
}

bool apply_axis_step(lighting_rig_t& io_rig, const variant_axis_t& in_axis) {
  const double l_v = in_axis.value_;
  switch (in_axis.axis_) {
    case variant_axis_kind::none:
      return false;

    case variant_axis_kind::key_direction: {
      if (l_v == 0.0) return false;
      auto* l_key = first_light_with_role(io_rig, light_role::key);
      if (l_key == nullptr) return false;
      double l_azimuth = std::fmod(l_key->offset_.azimuth_deg_ + l_v, 360.0);
      if (l_azimuth > 180.0) l_azimuth -= 360.0;
      if (l_azimuth < -180.0) l_azimuth += 360.0;
      l_key->offset_.azimuth_deg_ = l_azimuth;
      return true;
    }

    case variant_axis_kind::key_fill_ratio: {
      if (l_v <= 0.0) return false;
      // 当前比值走 key_fill_ratio_of()，与接缝 3 提示词里给模型的数字同一份定义。
      const auto l_current = key_fill_ratio_of(io_rig);
      if (!l_current.has_value()) return false;
      // 要让比值等于目标，就按 当前/目标 缩放补光。
      const double l_scale = *l_current / l_v;
      if (l_scale == 1.0) return false;
      for (auto& l_light : io_rig.lights_)
        if (l_light.role_ == light_role::fill) l_light.intensity_.value_ *= l_scale;
      return true;
    }

    case variant_axis_kind::kelvin: {
      if (l_v == 0.0) return false;
      for (auto& l_light : io_rig.lights_) {
        if (l_light.color_.mode_ != color_mode::temperature) continue;
        l_light.color_.kelvin_ = std::clamp(l_light.color_.kelvin_ + l_v, 1500.0, 15000.0);
      }
      io_rig.globals_.white_balance_.kelvin_ =
          std::clamp(io_rig.globals_.white_balance_.kelvin_ + l_v, 1500.0, 15000.0);
      // 即便所有灯都不是色温模式，全局白平衡也已经变了，所以这里恒为「改了」；
      // 真正没改的情况（已经在夹取边界上）由 map_candidate_result 的同一个候选检查兜住。
      return true;
    }

    case variant_axis_kind::hardness: {
      if (l_v == 0.0) return false;
      bool l_applied = false;
      for (auto& l_light : io_rig.lights_) {
        const double l_softness = std::clamp(l_light.shadows_.softness_ - 0.5 * l_v, 0.0, 1.0);
        const double l_radius   = std::max(0.0, l_light.shape_.source_radius_cm_ * (1.0 - 0.5 * l_v));
        if (l_softness != l_light.shadows_.softness_ || l_radius != l_light.shape_.source_radius_cm_)
          l_applied = true;
        l_light.shadows_.softness_       = l_softness;
        l_light.shape_.source_radius_cm_ = l_radius;
      }
      return l_applied;
    }

    case variant_axis_kind::ambient_level: {
      if (l_v == 0.0) return false;
      const double l_scale = std::clamp(1.0 + l_v, 0.0, 4.0);
      io_rig.globals_.environment_.sky_light_intensity_ =
          std::max(0.0, io_rig.globals_.environment_.sky_light_intensity_ * l_scale);
      for (auto& l_light : io_rig.lights_)
        if (l_light.role_ == light_role::ambient)
          l_light.intensity_.value_ = std::max(0.0, l_light.intensity_.value_ * l_scale);
      return true;
    }

    case variant_axis_kind::grading_strength: {
      if (l_v == 0.0) return false;
      // 只放大/缩小「偏离中性的量」：contrast/saturation 的中性值是 1，其余是 0。
      // 不碰 lights_ —— 这个轴是纯调色的自由度，动灯会让「调色 vs 布光」两个方向混在一起。
      const double l_scale = std::clamp(1.0 + l_v, 0.0, 3.0);
      auto& l_grading      = io_rig.globals_.grading_;
      l_grading.contrast_  = std::clamp(1.0 + (l_grading.contrast_ - 1.0) * l_scale, 0.0, 3.0);
      l_grading.saturation_ = std::clamp(1.0 + (l_grading.saturation_ - 1.0) * l_scale, 0.0, 3.0);
      l_grading.lift_       = std::clamp(l_grading.lift_ * l_scale, 0.0, 1.0);
      l_grading.vignette_   = std::clamp(l_grading.vignette_ * l_scale, 0.0, 1.0);
      l_grading.bloom_      = std::clamp(l_grading.bloom_ * l_scale, 0.0, 1.0);
      return true;
    }

    default:
      return false;
  }
}

std::vector<lighting_rig_t> expand_candidates(const lighting_rig_t& in_base, const iteration_axes_t& in_axes) {
  std::vector<lighting_rig_t> l_ret{};
  l_ret.reserve(1 + g_axis_step_budget);

  // 候选 0 是本轮基线：6 个候选是「1 个基线 + 5 个变异」，不是 6 个平行实验。
  // 有基线在场，美术才看得出「这次改动做了什么」，ops_ 也才有参照物（§15.8）。
  lighting_rig_t l_zero       = in_base;
  l_zero.variant_axis_        = variant_axis_t{};
  l_zero.variant_axis_.axis_  = variant_axis_kind::none;
  l_zero.variant_axis_.value_ = 0.0;
  l_zero.variant_axis_.label_ = "基线";
  l_zero.ops_.clear();
  l_ret.emplace_back(std::move(l_zero));

  for (const auto& l_axis : in_axes.axes_) {
    for (const double l_step : l_axis.steps_) {
      lighting_rig_t l_rig = in_base;
      variant_axis_t l_variant{};
      l_variant.axis_  = l_axis.axis_;
      l_variant.value_ = l_step;
      l_variant.label_ = l_axis.label_;
      l_rig.variant_axis_ = l_variant;
      apply_axis_step(l_rig, l_variant);
      l_rig.ops_.clear();
      l_ret.emplace_back(std::move(l_rig));
    }
  }
  return l_ret;
}

std::vector<light_op_t> diff_rigs(const lighting_rig_t& in_base, const lighting_rig_t& in_target) {
  std::vector<light_op_t> l_ops{};
  const std::string l_rationale =
      in_target.variant_axis_.axis_ == variant_axis_kind::none
          ? std::string{"基线"}
          : fmt::format("{} {}", magic_enum::enum_name(in_target.variant_axis_.axis_), in_target.variant_axis_.value_);

  // ---- 全局量：四块各对应一个 light_op_kind，只发真的变了的那几块 ----
  // patch 用「目标值整块」而不是增量：语义是 set，与 UE 侧「按 op 精确设置」一致，
  // 也与 §7.5 的「按 op 精确增删、不用 ClearAllLight」一致。
  if (nlohmann::json{in_base.globals_} != nlohmann::json{in_target.globals_}) {
    const std::pair<light_op_kind, const char*> l_blocks[] = {
        {light_op_kind::set_exposure, "exposure_"},
        {light_op_kind::set_white_balance, "white_balance_"},
        {light_op_kind::set_environment, "environment_"},
        {light_op_kind::set_grading, "grading_"},
    };
    const auto l_base_json = nlohmann::json{in_base.globals_};
    const auto l_tgt_json  = nlohmann::json{in_target.globals_};
    for (const auto& [l_kind, l_key] : l_blocks) {
      if (!l_base_json.contains(l_key) || l_base_json.at(l_key) == l_tgt_json.at(l_key)) continue;
      light_op_t l_op{};
      l_op.op_        = l_kind;
      l_op.patch_     = l_tgt_json.at(l_key);
      l_op.rationale_ = l_rationale;
      l_ops.emplace_back(std::move(l_op));
    }
  }

  // ---- 逐灯：以 id_ 对齐。展开器只改变量、不增删灯，所以两边 id 集合必然相同，
  //      这里也不会产生 add / remove（§6.6 的护栏正是这个意思）----
  const auto l_count = std::min(in_base.lights_.size(), in_target.lights_.size());
  for (std::size_t l_i = 0; l_i < l_count; ++l_i) {
    const auto& l_base_light = in_base.lights_.at(l_i);
    const auto& l_tgt_light  = in_target.lights_.at(l_i);
    if (l_base_light.id_ != l_tgt_light.id_) continue;
    // 逐子块比较（用类型化成员比较，patch 的键名是给执行器看的稳定名字）——
    // 这样 patch 里只有真的变了的子块，审计和执行器都看得清「这次动了什么」。
    nlohmann::json l_patch = nlohmann::json::object();
    if (nlohmann::json{l_base_light.offset_} != nlohmann::json{l_tgt_light.offset_})
      l_patch["offset"] = l_tgt_light.offset_;
    if (nlohmann::json{l_base_light.intensity_} != nlohmann::json{l_tgt_light.intensity_})
      l_patch["intensity"] = l_tgt_light.intensity_;
    if (nlohmann::json{l_base_light.color_} != nlohmann::json{l_tgt_light.color_})
      l_patch["color"] = l_tgt_light.color_;
    if (nlohmann::json{l_base_light.shape_} != nlohmann::json{l_tgt_light.shape_})
      l_patch["shape"] = l_tgt_light.shape_;
    if (nlohmann::json{l_base_light.shadows_} != nlohmann::json{l_tgt_light.shadows_})
      l_patch["shadows"] = l_tgt_light.shadows_;
    if (l_patch.empty()) continue;
    light_op_t l_op{};
    l_op.op_        = light_op_kind::modify;
    l_op.light_     = l_tgt_light.id_;
    l_op.patch_     = std::move(l_patch);
    l_op.rationale_ = l_rationale;
    l_ops.emplace_back(std::move(l_op));
  }
  return l_ops;
}

candidate_generation_result_t map_candidate_result(
    const nlohmann::json& in_result_json, const candidate_generation_request_t& in_request
) {
  DOODLE_CHICK_HTTP(
      in_result_json.is_object(), bad_gateway, "候选生成接缝收到了非对象的推理结果（{}），无法映射为候选",
      in_result_json.type_name()
  );

  // ---- 1) 物化基线 rig（模型给「灯怎么打」，服务端补 IR 细节）----
  auto l_base = build_base_rig(in_result_json);

  // ---- 2) 服务端钉死：身份与来源字段一律不取模型的值 ----
  l_base.session_id_       = in_request.session_id_;
  l_base.spec_id_          = in_request.spec_.spec_id_;
  l_base.iteration_        = in_request.iteration_;
  l_base.candidate_index_  = 0;
  l_base.rig_id_           = uuid{};  // 由调用方一次性分配（接缝不落库，也不需要它唯一）
  l_base.baseline_level_   = in_request.baseline_level_;
  l_base.variant_axis_     = variant_axis_t{};
  l_base.ops_.clear();
  l_base.provenance_          = provenance_t{};
  l_base.provenance_.model_   = doodle_config::g_ark_inference_model;
  // prompt_hash_ 是「这份 rig 是从哪份输入推出来的」的审计指纹。用同一份 input_text 再算一次：
  // 这里没有别的随机源（seed_ 恒 0），所以同一输入必然得到同一指纹（§8.5 第 1 条）。
  l_base.provenance_.prompt_hash_ = std::to_string(std::hash<std::string>{}(build_candidates_input_text(in_request)));
  l_base.provenance_.seed_        = 0;

  // ---- 3) §9.1 的灵敏度前提：没有主光，两个最常用的轴会变成空操作 ----
  DOODLE_CHICK_HTTP(
      first_light_with_role(l_base, light_role::key) != nullptr, bad_gateway,
      "候选基线 rig 里没有 role=key 的主光：key_direction / key_fill_ratio 两个轴都会变成空操作，"
      "§9.1 要求的「候选之间真的在目标上有差异」无法成立"
  );

  // ---- 4) 轴计划：不合法就整体回退默认计划（不抛错）----
  iteration_axes_t l_axes{};
  std::string l_strategy{};
  if (const auto l_reason = parse_axis_plan(in_result_json, l_axes, l_strategy); l_reason.has_value()) {
    l_axes            = default_axis_plan(in_request.spec_);
    const auto l_head = l_strategy.empty() ? std::string{} : fmt::format("模型策略：{}｜", l_strategy);
    l_axes.strategy_  = fmt::format("{}模型给的轴计划被弃用：{}", l_head, *l_reason);
    if (l_axes.strategy_.size() > g_strategy_max_len) l_axes.strategy_.resize(g_strategy_max_len);
  } else {
    l_axes.strategy_ = l_strategy;
  }

  // ---- 5) OFAT 展开 ----
  auto l_rigs = expand_candidates(l_base, l_axes);

  // ---- 6) 自洽（契约 3）：钉死身份、写 ops_、确认每个变异候选真的与基线不同 ----
  for (std::size_t l_i = 0; l_i < l_rigs.size(); ++l_i) {
    auto& l_rig            = l_rigs.at(l_i);
    l_rig.session_id_      = in_request.session_id_;
    l_rig.spec_id_         = in_request.spec_.spec_id_;
    l_rig.iteration_       = in_request.iteration_;
    l_rig.baseline_level_  = in_request.baseline_level_;
    l_rig.candidate_index_ = static_cast<std::int32_t>(l_i);
    l_rig.rig_id_          = uuid{};
    l_rig.provenance_      = l_base.provenance_;
    l_rig.ops_             = diff_rigs(l_base, l_rig);
    if (l_i == 0) continue;
    // §9.1 的「候选真的在目标上有差异」的最小实现：沿某个轴变了、但 rig 一个字节都没变，
    // 说明这条轴在当前 rig 上不生效（缺 fill 灯、单位不可比、已经在夹取边界上……）。
    // 返回 6 张一样的图比一次失败更糟 —— 那会让美术以为「这几个方向看起来一样」。
    DOODLE_CHICK_HTTP(
        !(rig_substance(l_rig) == rig_substance(l_base)), bad_gateway,
        "候选 {} 沿轴 {} = {} 展开后与基线完全相同（这条轴在当前 rig 上不生效）",
        l_i, magic_enum::enum_name(l_rig.variant_axis_.axis_), l_rig.variant_axis_.value_
    );
  }

  candidate_generation_result_t l_ret{};
  l_ret.rigs_ = std::move(l_rigs);
  l_ret.axes_ = std::move(l_axes);
  // 契约 3 的另一半：调用方还会再校验一次（500），这里先保证自己自洽。
  DOODLE_CHICK_HTTP(
      l_ret.rigs_.size() == static_cast<std::size_t>(g_candidate_count), internal_server_error,
      "OFAT 展开后得到 {} 个候选，方案 §6.6 要求恰好 {}", l_ret.rigs_.size(), g_candidate_count
  );
  return l_ret;
}

// ---------------------------------------------------------------------------
// 三条接缝共用的样板：触发人 → AI 工作室 → 客户端，以及推理结果的成败判定
// ---------------------------------------------------------------------------
// 抽出来的理由很实际：generate_spec / generate_candidates / structure_feedback 会逐字
// 重复同一段约 60 行，而其中「什么算可解释的失败（400）」是一条**判定** ——
// 三份拷贝里任何一份漂移，都会让同一个失败在不同接缝上给出不同的状态码。
namespace {

struct resolved_inference_t {
  std::shared_ptr<http::seedance2::ai_client_base> client_{};
  uuid studio_id_{};
};

/// 触发人 → person → ai_studio → 推理客户端。
///
/// 用 select + to_optional 而不是 get_by_uuid<T>：后者内部是 to_single()，「查不到」会变成
/// 一条 sqlite 异常（HTTP 500），而这里应当是 400 语义。
/// 每次推理都**新建**客户端：http_client_ssl 内部只有一个 socket 且无锁（§15.6 第 6 条）。
/// in_what 只用于措辞（出错信息要指明哪条接缝），不参与任何判定。
resolved_inference_t make_seam_client(
    sqlite_database& in_sql, const uuid& in_operator_id, std::string_view in_what
) {
  DOODLE_CHICK_HTTP(
      !in_operator_id.is_nil(), bad_request, "{} 缺少触发人（operator_id），无法解析 AI 工作室与计费主体",
      in_what
  );
  const auto l_person =
      select(in_sql).columns(object<person>()).from<person>().where(c(&person::uuid_id_) == in_operator_id)()
          .to_optional();
  DOODLE_CHICK_HTTP(
      l_person.has_value(), bad_request, "{} 找不到触发人 {}（person 不存在），无法解析 AI 工作室", in_what,
      in_operator_id
  );
  DOODLE_CHICK_HTTP(
      !l_person->ai_studio_id_.is_nil(), bad_request, "{} 的触发人 {} 未关联 AI 工作室，无法调用推理接口",
      in_what, in_operator_id
  );
  const auto l_studio = select(in_sql)
                            .columns(object<ai_studio>())
                            .from<ai_studio>()
                            .where(c(&ai_studio::uuid_id_) == l_person->ai_studio_id_)()
                            .to_optional();
  DOODLE_CHICK_HTTP(
      l_studio.has_value(), bad_request, "{} 找不到 AI 工作室 {}（配置可能已被删除）", in_what,
      l_person->ai_studio_id_
  );

  resolved_inference_t l_out{};
  l_out.studio_id_ = l_studio->uuid_id_;
  l_out.client_    = http::seedance2::make_inference_client(*l_studio);
  return l_out;
}

/// 一次推理的成败判定。重试全在客户端内做完了（g_ark_inference_max_retry），这里不重试。
/// 内容违规 → 400（可解释：改提示词/换图能解决），其余失败 → 502；
/// result_json_ 非 object（含 null）也是 502 —— 输出不合解析不是调用方的错。
void chick_inference_ok(const http::seedance2::ai_client_base::run_task_result_t& in_result, std::string_view in_what) {
  const auto l_brief_of = [&in_result]() {
    return in_result.result_text_.substr(0, std::min<std::size_t>(200, in_result.result_text_.size()));
  };
  if (in_result.status_ != seedance2::task_status::succeeded) {
    const auto l_brief = l_brief_of();
    if (http::seedance2::seedance2_client::is_sensitive_content_error(in_result.data_response_)) {
      throw_exception(doodle_error{
          boost::beast::http::status::bad_request, "{} 被内容安全策略拒绝：{}", in_what,
          l_brief.empty() ? in_result.error_code_ : l_brief
      });
    }
    throw_exception(doodle_error{
        boost::beast::http::status::bad_gateway, "{} 推理失败（status={}, error_code={}, 超时={}）：{}", in_what,
        magic_enum::enum_name(in_result.status_), in_result.error_code_, in_result.is_timeout_, l_brief
    });
  }
  // result_json_ 由 parse_inference_response 填好（内部已做过 strip_code_fence + parse），
  // 调用方**不要**二次解析 result_text_。
  if (!in_result.result_json_.is_object()) {
    throw_exception(doodle_error{
        boost::beast::http::status::bad_gateway, "{} 的模型输出不是合法 JSON 对象：{}", in_what, l_brief_of()
    });
  }
}

}  // namespace

boost::asio::awaitable<spec_generation_result_t> generate_spec(
    sqlite_database& in_sql, const spec_generation_request_t& in_request
) {
  // ---- 1) 统计聚合（纯计算，无 IO；无可用统计也不抛，走 prompt_only 降级）----
  const auto l_agg = aggregate_reference_stats(in_request.references_);

  // ---- 2) 触发人 → AI 工作室 → 客户端 ----
  auto l_resolved = make_seam_client(in_sql, in_request.operator_id_, "generate_spec");

  // ---- 3) 组装任务与归一化信封 ----
  seedance2::task l_task{};
  l_task.type_                  = seedance2::task_type::inference;  // not_null，必须显式赋值
  l_task.backend_               = seedance2::task_backend::seedance2;  // 同上
  l_task.user_id_               = in_request.operator_id_;  // 计费主体 + 签名 URL 的 uid
  l_task.ai_studio_id_          = l_resolved.studio_id_;
  l_task.project_uuid_id_       = in_request.project_id_;
  l_task.ai_light_iteration_id_ = uuid{};                   // 蒸馏阶段还没有轮次，留 nil
  l_task.text_prompt_           = in_request.prompt_text_;   // 去重与前台展示都读它
  l_task.completion_tokens_     = doodle_config::g_ark_inference_default_consumed_tokens;
  // 结构体 -> json 走 inference_request_t 的 to_json, 落库键集与手写 json 时完全一致
  l_task.data_request_          = build_spec_request(in_request, l_agg);

  // run_inference_task 自己一个事务里落终态任务行 + 扣费（§15.6 的方案 A）。
  // 这是本接缝唯一的副作用，且**有意保留**：推理确实发生了，必须留痕计费 ——
  // 否则调用方回滚会话状态时会连带把 person_token 的扣费抹掉（契约 2 的受控例外）。
  auto l_result = co_await http::seedance2::run_inference_task(l_task, in_sql, l_resolved.client_);

  // ---- 4) 成败判定（内容违规 400 / 其它失败 502 / 输出非 object 502）----
  chick_inference_ok(l_result, "generate_spec");

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
    sqlite_database& in_sql, const candidate_generation_request_t& in_request
) {
  // ---- 1) 触发人 → AI 工作室 → 客户端 ----
  auto l_resolved = make_seam_client(in_sql, in_request.operator_id_, "generate_candidates");

  // ---- 2) 组装任务与归一化信封 ----
  seedance2::task l_task{};
  l_task.type_                  = seedance2::task_type::inference;       // not_null，必须显式赋值
  l_task.backend_               = seedance2::task_backend::seedance2;    // 同上
  l_task.user_id_               = in_request.operator_id_;               // 计费主体 + 签名 URL 的 uid
  l_task.ai_studio_id_          = l_resolved.studio_id_;
  l_task.project_uuid_id_       = in_request.project_id_;
  // 与 generate_spec 的区别：调用方在调用**之前**就把 iteration 的 uuid 铸造好了，
  // 所以这条推理任务能反查到「哪一次推理生成了第 N 轮候选」。
  l_task.ai_light_iteration_id_ = in_request.iteration_id_;
  l_task.text_prompt_           = in_request.prompt_text_;
  l_task.completion_tokens_     = doodle_config::g_ark_inference_default_consumed_tokens;
  // 结构体 -> json 走 inference_request_t 的 to_json，落库键集与手写 json 时完全一致
  l_task.data_request_          = build_candidates_request(in_request);

  // run_inference_task 自己一个事务里落终态任务行 + 扣费（§15.6 的方案 A）。
  // 这是本接缝唯一的副作用，且**有意保留**：推理确实发生了，必须留痕计费 ——
  // 否则调用方回滚会话状态时会连带把 person_token 的扣费抹掉（契约 2 的受控例外）。
  auto l_result = co_await http::seedance2::run_inference_task(l_task, in_sql, l_resolved.client_);

  // ---- 3) 成败判定（内容违规 400 / 其它失败 502 / 输出非 object 502）----
  chick_inference_ok(l_result, "generate_candidates");

  // ---- 4) 映射（物化基线 + 轴计划校验/回退 + OFAT 展开 + ops diff + 自洽检查）----
  auto l_ret = map_candidate_result(l_result.result_json_, in_request);

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 第 {} 轮候选生成完成：候选 {} 个，轴 {} 条（{}），token in/out/total={}/{}/{}",
      in_request.session_id_, in_request.iteration_, l_ret.rigs_.size(), l_ret.axes_.axes_.size(),
      l_ret.axes_.strategy_, l_result.input_tokens_, l_result.output_tokens_, l_result.total_tokens_
  );

  co_return l_ret;
}

// ===========================================================================
// 接缝 3：自由文本反馈 → 结构化轴调整（方案 §6.2 / §13.1 第 9 项）
// ===========================================================================
//
// 与前两条接缝同样的分层：纯函数组信封/映射 + 一次真实网络调用。
// 产出比它们窄得多 —— 不是一套 rig、也不是一份方案，只是「往哪个方向动、动多少」。
// 这个窄产出正是「滑块是默认通道、文本是补充通道」的实现形态：两条通道最终写进
// 同一个 std::map<std::string,double>，再由 advance_session:307-319 变成 axis_step_t。

namespace {

/// 模型最多能给几条轴。**这不是审美选择而是硬约束**：下一轮的 parse_axis_plan() 只接受
/// 1..g_axis_max_count(2) 条，而 feedback_axes_ 是「这些轴必须出现在你的 axes 里」的
/// **约束** —— 给 3 条，模型无论如何都满足不了，轴计划必然被弃用，文本通道静默失效。
constexpr std::size_t g_feedback_axis_max_count = g_axis_max_count;

/// 一个方向的文字说明（reason）会作为 axis_step_t::label_ 落库并回显给美术。
/// 它是 TEXT 列，模型可以长篇大论，截断避免一次输出撑爆这一列。
constexpr std::size_t g_feedback_label_max_len = 200;

/// 6.000 -> "6"，0.350 -> "0.35"，-800.000 -> "-800"。
/// 给模型和美术看的数字不该带尾随零：提示词里写 "key:fill = 6.000" 会让人以为那是测量值。
std::string trim_number(double in_value) {
  std::string l_out = fmt::format("{:.3f}", in_value);
  if (l_out.find('.') == std::string::npos) return l_out;
  l_out.erase(l_out.find_last_not_of('0') + 1);
  if (!l_out.empty() && l_out.back() == '.') l_out.pop_back();
  return l_out;
}

}  // namespace

std::string build_feedback_instructions() {
  std::string l_out{};
  l_out +=
      "你是电影灯光指导的**翻译**环节：美术刚看完一轮渲染，用一句大白话说了他想要什么。\n"
      "你要把这句大白话翻译成灯光轴上的**方向**，由下一轮的排轴环节把它变成具体的档位。\n"
      "你**不**负责出灯、也**不**负责出档位表 —— 只判断「往哪个轴、往哪边、动多少」。\n\n";
  // 与 apply_axis_step 同一份口径（§15.9）：模型按它给方向，服务端按同一套公式应用。
  l_out += "【轴值语义】\n";
  l_out += g_axis_value_semantics_text;
  l_out += "\n【约束】\n";
  l_out += fmt::format(
      "- 每条轴只给**一个**数值（value）：这是下一轮的中心值，不是档位表。\n"
      "- 最多给 {} 条轴。下一轮只接受不超过 {} 条 —— 给多了整份轴计划会被丢弃，你的翻译就白做了。\n"
      "- 不要给 none（那不是方向）；不要给 0（0 等于「没有变化」，同样读不出方向）。\n"
      "- key_fill_ratio 是**绝对**比值，必须参考下面给出的「当前 key:fill 比」来定新值；\n"
      "  其余五个轴都是**增量**，正负决定方向。\n"
      "- 一个方向只由一条轴承担：别让两条轴说同一件事（比如既抬环境光又压对比）。\n"
      "- 美术没有给出任何方向时（只是说「再调调看」「换一批」），axes 就给**空数组** ——\n"
      "  不要为了凑数编一条，那会让下一轮往一个错的方向收缩。\n\n"
      "【输出要求】\n"
      "只输出符合给定 JSON Schema 的 JSON。不要输出解释文字，不要用 Markdown 代码块。\n"
      "understanding 用一句中文说清你的理解（给人工核对用）。\n",
      g_feedback_axis_max_count, g_feedback_axis_max_count
  );
  return l_out;
}

nlohmann::json build_feedback_json_schema() {
  nlohmann::json l_axis_item{};
  l_axis_item["type"]                 = "object";
  l_axis_item["additionalProperties"] = false;
  l_axis_item["properties"]           = nlohmann::json{
      {"axis", make_axis_enum_schema()},
      {"value", {{"type", "number"}}},
      {"reason", {{"type", "string"}}},
  };
  l_axis_item["required"] = nlohmann::json::array({"axis", "value", "reason"});

  nlohmann::json l_axes{};
  l_axes["type"]  = "array";
  l_axes["items"] = std::move(l_axis_item);
  // 只设上限、**不设 minItems**：空数组是「美术没给出方向」的合法表达（决定 D13），
  // 加了 minItems 会逼模型编一条不存在的方向。
  l_axes["maxItems"] = g_feedback_axis_max_count;

  nlohmann::json l_schema{};
  l_schema["type"]                 = "object";
  l_schema["additionalProperties"] = false;
  l_schema["properties"]           = nlohmann::json{
      {"understanding", {{"type", "string"}, {"description", "你对美术这句话的理解，一句话"}}},
      {"axes", std::move(l_axes)},
  };
  l_schema["required"] = nlohmann::json::array({"understanding", "axes"});
  return l_schema;
}

std::string build_feedback_input_text(const feedback_structuring_request_t& in_request) {
  std::string l_out{};
  // ---- 1) 美术的原话（逐字）----
  l_out += "【美术的原话（权威，一字不改）】\n";
  l_out += in_request.feedback_text_;
  l_out += "\n";

  // ---- 2) 这次判断的决定 + 选中的候选 ----
  // 选中候选的**序号**（candidate_index_，从 0 开始）而不是 uuid：原话里的「第 3 个」
  // 指的就是界面上的排序，模型与美术必须看到同一个编号。
  std::int32_t l_selected_index = -1;
  if (!in_request.selected_candidate_id_.is_nil())
    for (const auto& l_cand : in_request.candidates_)
      if (l_cand.uuid_id_ == in_request.selected_candidate_id_) l_selected_index = l_cand.candidate_index_;

  l_out += "\n【这次判断的决定】\n";
  l_out += fmt::format("决定 = {}", magic_enum::enum_name(in_request.decision_));
  if (l_selected_index >= 0) {
    l_out += fmt::format("；美术选中了候选 #{}", l_selected_index);
  } else if (!in_request.selected_candidate_id_.is_nil()) {
    l_out += fmt::format("；美术选中了候选 {}（不在这轮的候选列表里）", in_request.selected_candidate_id_);
  } else {
    l_out += "；美术没有指定候选";
  }
  l_out += "\n";

  // ---- 3) Spec 目标（与前两条接缝同一个投影函数）----
  l_out += "\n【已确认的灯光方案（LightingSpec，人工已确认）】\n";
  l_out += fmt::format("降级级别 = {}\n", magic_enum::enum_name(in_request.spec_.degrade_level_));
  l_out += describe_spec_brief(in_request.spec_);
  l_out += "\n";

  // ---- 4) 本轮 6 个候选的对照结构 ----
  // OFAT 不变量（§15.8）：候选 0 是基线，其余各自只沿一条轴变一个档位 ——
  // 所以「基线 rig + 每个候选的轴档位」足以描述全部 6 张图，不必展开 6 套完整 rig
  // （那会把提示词撑大 6 倍，而多出来的信息只有重复）。
  l_out += fmt::format("\n【本轮（第 {} 轮）这 6 个候选是怎么来的】\n", in_request.iteration_);
  const lighting_rig_t l_base_rig =
      in_request.candidates_.empty() ? lighting_rig_t{} : in_request.candidates_.front().get_rig();
  if (in_request.candidates_.empty()) {
    l_out += "本轮候选还没落库。请只依据 Spec 与美术的原话判断方向。\n";
  } else {
    l_out += fmt::format("#0 基线：{}\n", describe_rig_brief(l_base_rig));
    for (const auto& l_cand : in_request.candidates_) {
      if (l_cand.candidate_index_ == 0) continue;
      const auto& l_axis = l_cand.get_rig().variant_axis_;
      l_out += fmt::format(
          "#{}  轴={}  值={}  {}\n", l_cand.candidate_index_, magic_enum::enum_name(l_axis.axis_),
          trim_number(l_axis.value_), l_axis.label_
      );
    }
  }

  // ---- 5) 选中候选的完整 rig ----
  if (l_selected_index >= 0) {
    for (const auto& l_cand : in_request.candidates_) {
      if (l_cand.candidate_index_ != l_selected_index) continue;
      l_out += "\n【美术选中的那套（原话最可能指的就是它）】\n";
      l_out += describe_rig_brief(l_cand.get_rig());
      l_out += "\n";
      break;
    }
  }

  // ---- 6) 当前 key:fill 比（绝对量轴必须知道现状）----
  if (!in_request.candidates_.empty()) {
    const auto l_ratio = key_fill_ratio_of(l_base_rig);
    l_out += "\n【当前 key:fill 比】\n";
    if (l_ratio.has_value()) {
      l_out += fmt::format("{}（基线 rig 的主光强度 ÷ 全部补光强度之和）\n", trim_number(*l_ratio));
    } else {
      l_out += "算不出来（缺主光 / 缺补光 / 主光与补光强度单位不同）—— 这时不要给 key_fill_ratio 轴。\n";
    }
  }

  // ---- 7) 滑块已经给出的方向 ----
  l_out += "\n【美术已经用滑块给出的方向】\n";
  if (in_request.slider_axes_.empty()) {
    l_out += "没有（美术只写了字）。\n";
  } else {
    for (const auto& [l_name, l_value] : in_request.slider_axes_)
      l_out += fmt::format("{} = {}\n", l_name, trim_number(l_value));
    // 决策 5 + §15.9 决定 D9：滑块是默认通道，同一轴上滑块优先。
    l_out += "这些轴美术已经自己拖过了，**不要再给**（给了也会被滑块的数值覆盖）。\n";
  }
  l_out += "\n";
  return l_out;
}

http::seedance2::inference_request_t build_feedback_request(const feedback_structuring_request_t& in_request) {
  http::seedance2::inference_request_t l_req{};
  l_req.instructions_       = build_feedback_instructions();
  l_req.input_text_         = build_feedback_input_text(in_request);
  l_req.temperature_        = 0.2;
  l_req.json_schema_        = build_feedback_json_schema();
  l_req.json_schema_name_   = "lighting_feedback";
  l_req.json_schema_strict_ = true;
  // 刻意不发（与 build_candidates_request 同样的三条理由）：
  //   * images / image_detail —— 固定这一轮的候选图在浏览器里，没进本请求；没有 images
  //     还发 image_detail 就是给「不存在的输入」配参数（§15.6 坑 21）。
  //   * max_output_tokens —— 缺省 4096 会截断，而截断的表现（result_json_ 变 null）
  //     与「schema 被拒」无法区分。
  //   * thinking —— 缺省 disabled；结构化任务开 thinking 会让 output[0] 变成 reasoning。
  return l_req;
}

std::vector<axis_step_t> map_feedback_result(const nlohmann::json& in_result_json) {
  // ---- 形状违规：抛错（协议被无视）----
  DOODLE_CHICK_HTTP(
      in_result_json.is_object(), bad_gateway, "文本反馈结构化的模型输出不是 JSON 对象：{}",
      in_result_json.is_null() ? std::string{"null"} : in_result_json.dump().substr(0, 200)
  );
  DOODLE_CHICK_HTTP(
      in_result_json.contains("understanding") && in_result_json.at("understanding").is_string(), bad_gateway,
      "文本反馈结构化的模型输出缺 understanding（或它不是字符串）"
  );
  DOODLE_CHICK_HTTP(
      in_result_json.contains("axes") && in_result_json.at("axes").is_array(), bad_gateway,
      "文本反馈结构化的模型输出缺 axes（或它不是数组）"
  );
  const auto& l_axes = in_result_json.at("axes");
  DOODLE_CHICK_HTTP(
      l_axes.size() <= g_feedback_axis_max_count, bad_gateway,
      "文本反馈结构化给了 {} 条轴，超过下一轮能接受的 {} 条", l_axes.size(), g_feedback_axis_max_count
  );

  // ---- 语义无效：在源头丢弃 ----
  // 为什么不抛错：unknown 轴名与 advance_session:314 的既有容错一致；none / 0 是「没有变化」
  // 的合法语义，但它一旦传下去，下一轮 advance_session 会把它变成 axis_step_t，
  // apply_axis_step() 对 0/none 恒返回 false，最终以「轴无实质变化」抛 502 ——
  // 那时病根与表现之间隔了一次**人工判断**（美术得先白判一轮）。丢弃在这里是最短的修复路径。
  //
  // 这里**不记日志**：本函数是离线可测的纯函数，而单测宿主没有 HTTP logger
  // （同 ai_light_stats.cpp 的 aggregate_reference_stats）。丢弃的条数由接缝本体
  // 用「模型给了几条 vs 我们留下几条」在日志里体现。
  std::vector<axis_step_t> l_out{};
  for (const auto& l_item : l_axes) {
    DOODLE_CHICK_HTTP(
        l_item.is_object() && l_item.contains("axis") && l_item.at("axis").is_string(), bad_gateway,
        "文本反馈结构化的 axes 元素缺 axis（或它不是字符串）"
    );
    const auto l_name = l_item.at("axis").get<std::string>();
    const auto l_kind = magic_enum::enum_cast<variant_axis_kind>(l_name);
    if (!l_kind.has_value() || *l_kind == variant_axis_kind::none) continue;
    // 同一条轴出现两次时取第一个：保持确定性，别让「最后一个赢」取决于模型输出顺序。
    const bool l_dup = std::any_of(
        l_out.begin(), l_out.end(), [&l_kind](const axis_step_t& in_s) { return in_s.axis_ == *l_kind; }
    );
    if (l_dup) continue;
    DOODLE_CHICK_HTTP(
        l_item.contains("value") && l_item.at("value").is_number(), bad_gateway,
        "文本反馈结构化的轴 {} 缺 value（或它不是数字）", l_name
    );
    const double l_value = l_item.at("value").get<double>();
    if (!std::isfinite(l_value) || l_value == 0.0) continue;
    // key_fill_ratio 是比值，负值/零没有意义（apply_axis_step 对 <= 0 也直接返回 false）。
    if (*l_kind == variant_axis_kind::key_fill_ratio && l_value <= 0.0) continue;

    axis_step_t l_step{};
    l_step.axis_ = *l_kind;
    // 每条轴**恰好一个值**（决定 D11）：调用方只取 front() 当下一轮的中心值，
    // 完整档位表由下一轮 generate_candidates 的 axes_ 决定。
    l_step.steps_ = {l_value};
    auto l_reason = std::string{};
    if (l_item.contains("reason") && l_item.at("reason").is_string()) l_reason = l_item.at("reason").get<std::string>();
    if (l_reason.empty()) l_reason = std::string{magic_enum::enum_name(*l_kind)};
    l_step.label_ = l_reason.substr(0, std::min<std::size_t>(g_feedback_label_max_len, l_reason.size()));
    l_out.emplace_back(std::move(l_step));
  }
  return l_out;
}

boost::asio::awaitable<std::vector<axis_step_t>> structure_feedback(
    sqlite_database& in_sql, const feedback_structuring_request_t& in_request
) {
  // ---- 1) 触发人 → AI 工作室 → 客户端 ----
  auto l_resolved = make_seam_client(in_sql, in_request.operator_id_, "structure_feedback");

  // ---- 2) 组装任务与归一化信封 ----
  seedance2::task l_task{};
  l_task.type_             = seedance2::task_type::inference;     // not_null，必须显式赋值
  l_task.backend_          = seedance2::task_backend::seedance2;  // 同上
  l_task.user_id_          = in_request.operator_id_;             // 计费主体 + 签名 URL 的 uid
  l_task.ai_studio_id_     = l_resolved.studio_id_;
  l_task.project_uuid_id_  = in_request.project_id_;
  // 与另两条接缝的区别：这里记的是「哪一轮的反馈被结构化了」，用于把一次计费对回一次人工判断。
  l_task.ai_light_iteration_id_ = in_request.iteration_id_;
  l_task.text_prompt_           = fmt::format(
      "AI 灯光会话 {} 第 {} 轮的文本反馈", in_request.session_id_, in_request.iteration_
  );
  l_task.completion_tokens_ = doodle_config::g_ark_inference_default_consumed_tokens;
  l_task.data_request_      = build_feedback_request(in_request);

  // run_inference_task 自己一个事务里落终态任务行 + 扣费（§15.6 的方案 A）。
  // 这是本接缝唯一的副作用，且**有意保留**：推理确实发生了，必须留痕计费 ——
  // 否则调用方降级时会把 person_token 的扣费一起抹掉（契约 2 的受控例外）。
  auto l_result = co_await http::seedance2::run_inference_task(l_task, in_sql, l_resolved.client_);

  // ---- 3) 成败判定（内容违规 400 / 其它失败 502 / 输出非 object 502）----
  chick_inference_ok(l_result, "structure_feedback");

  // ---- 4) 映射（形状违规抛错；语义无效的轴丢弃）----
  auto l_steps = map_feedback_result(l_result.result_json_);

  // 「模型给了几条 vs 我们留下几条」是唯一能观测到「轴被丢弃」的地方（map_feedback_result
  // 是纯函数、不记日志），所以这里连原始 axes 一起记下来。
  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(),
      "AI 灯光会话 {} 第 {} 轮文本反馈结构化完成：模型给 {} 条轴、留下 {} 条，理解=\"{}\"，原始 axes={}，"
      "token in/out/total={}/{}/{}",
      in_request.session_id_, in_request.iteration_, l_result.result_json_.at("axes").size(), l_steps.size(),
      l_result.result_json_.value("understanding", std::string{}), l_result.result_json_.at("axes").dump(),
      l_result.input_tokens_, l_result.output_tokens_, l_result.total_tokens_
  );

  co_return l_steps;
}

}  // namespace doodle::ai_light
