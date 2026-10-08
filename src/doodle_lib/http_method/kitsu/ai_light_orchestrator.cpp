//
// AI 灯光：轮次编排与状态机实现
//
// 对应 docs/ue_ai_lighting_plan.md §10.3（状态流转）与 §10.4（派发契约）。
//
// 本文件只装**状态机**：advance_session / apply_judgement / mark_iteration_rendered /
// build_session_detail，以及它自己的两个小助手。三条「接缝」（generate_spec /
// generate_candidates / structure_feedback）、rig 纯运算、共用部件与 JSON 架构层都已按
// §15.11 拆到同目录的独立文件：
//   * `ai_light_seam_spec.{h,cpp}`        —— 蒸馏 Spec（§15.7）
//   * `ai_light_seam_candidates.{h,cpp}`  —— 基线 rig + 轴计划 + OFAT 展开（§15.8）
//   * `ai_light_seam_feedback.{h,cpp}`    —— 自由文本 → 轴方向（§15.9）
//   * `ai_light_rig_ops.{h,cpp}`          —— 轴施加 / OFAT 展开 / ops diff / key:fill 比
//   * `ai_light_seam_common.{h,cpp}`      —— 接缝共用提示词片段 + 描述 + 推理管道
//   * `ai_light_json_validation.{h,cpp}`  —— schema 建造器 + 严格/宽松读取器 + 架构诊断（方案 §3）
// 对外仍然只需包含伞形头 `ai_light_orchestrator.h`。接缝的契约（抛即回滚、不得留下非幂等的
// 部分写入、返回值必须自洽）见该头顶部。
//

#include "ai_light_orchestrator.h"

#include <doodle_core/configure/static_value.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/server_task_info.h>

#include "ai_light_json_validation.h"
#include "ai_light_rig_ops.h"
#include "ai_light_seam_common.h"


// 注：本文件曾经还包含 app_base.h / boost/asio/co_spawn.hpp / steady_timer.hpp /
// use_awaitable.hpp —— 它们只服务于那个已删除的 4 小时挂起扫描器（常驻后台协程）。
// 过期现在是纯派生标志，这里不再有任何后台任务，所以一并去掉：
// 留着这些 include 会让人以为这儿还跑着一个循环。
// 同理，entity_path.h 与 project.h 也已去掉：它们只服务于那个已删除的
// resolve_session_paths（产物路径改由 kitsu_ctx_t 按会话 uuid 生成，不查库）。
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

/// failure_reason_ 的长度上限。失败原因要给人看、要给前端显示，
/// 但 boost 的诊断信息可能带整条调用栈，不截断会把这列撑成日志垃圾场。
constexpr std::size_t g_failure_reason_max = 512;

std::string brief_text(std::string in_what) {
  if (in_what.size() <= g_failure_reason_max) return in_what;
  in_what.resize(g_failure_reason_max);
  in_what += "...(已截断)";
  return in_what;
}

std::string brief_error(const std::exception& in_err) { return brief_text(in_err.what()); }

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
// 状态机：推进
// ===========================================================================

namespace {

/// 第一步：draft / awaiting_spec_confirm → 蒸馏出 LightingSpec。
///
/// 从 advance_session 里拆出来（方案 §6）：这一段本身就是「一次接缝调用 + 一个事务」的完整
/// 闭环，与下面出候选那一段没有共享状态 —— 唯一的共享是 `io_session`（状态与
/// `failure_reason_` 由本函数直接改，因为回滚要用到它们）。
///
/// **本函数不捕获任何异常**：接缝契约 1 说「抛 = 本轮无法推进」，回滚与重抛由调用方统一做。
/// 这样两个分支的失败路径只有一份实现，也不会出现「分支里吞掉了异常」这种岔路。
///
/// 两个入口状态，同一个意图：产出（或重出）当前版本的方案。
/// 这不违反「一个状态只能有一个后继步骤」：awaiting_spec_confirm 上还有一个 /spec/confirm，
/// 但它会**先把状态改成 generating** 再调 advance_session（ai_light.cpp 的 spec_confirm_route），
/// 所以落到本分支的只有「重出」这一个意图。
boost::asio::awaitable<void> advance_session_spec_branch(
    sqlite_database& in_sql, ai_light_session& io_session, const uuid& in_operator
) {
  auto l_refs = in_sql.get_ai_light_references(io_session.uuid_id_);

  DOODLE_CHICK_HTTP(
      !in_operator.is_nil(), bad_request,
      "会话 {} 无法确定触发人（无 operator_id 且会话无 submitter），无法解析 AI 工作室与计费主体", io_session.uuid_id_
  );

  spec_generation_request_t l_req{};
  l_req.session_id_  = io_session.uuid_id_;
  l_req.project_id_  = io_session.project_id_;
  l_req.operator_id_ = in_operator;
  l_req.prompt_text_ = io_session.prompt_text_;
  l_req.references_  = l_refs;
  l_req.degrade_level_ =
      references_have_stats(l_refs) ? spec_degrade_level::stats_only : spec_degrade_level::prompt_only;
  if (auto l_prev = in_sql.get_current_ai_light_spec(io_session.uuid_id_); l_prev.has_value())
    l_req.previous_spec_ = l_prev->to_spec();

  auto l_res       = co_await generate_spec(in_sql, l_req);

  // revision = 现有最大 + 1。get_ai_light_specs 按 revision 升序返回。
  auto l_history   = in_sql.get_ai_light_specs(io_session.uuid_id_);
  const auto l_rev = l_history.empty() ? 1 : l_history.back().revision_ + 1;

  ai_light_spec l_spec{};
  l_spec.session_id_ = io_session.uuid_id_;
  l_spec.revision_   = l_rev;
  l_spec.from_spec(l_res.spec_);
  l_spec.set_evidence(l_res.evidence_);
  l_spec.set_reference_stats(l_res.reference_stats_);
  l_spec.set_conflict_resolution(l_res.conflict_resolution_);
  // 降级级别以**接缝按事实判定的结果**为准，不用调用方那个预判（决定 D4）：
  // 「参考图有 stats 行但置信度全 0」或「全部图被人工排除」时，调用方看
  // references_have_stats() 会以为是 stats_only，而实际聚合出一无所有。
  l_spec.degrade_level_      = l_res.spec_.degrade_level_;
  // 新出的方案一律未确认：人工闸门 1 必须重新过一遍。
  l_spec.human_confirmed_    = false;
  l_spec.confirmed_by_       = uuid{};
  l_spec.confirmed_at_       = std::nullopt;

  io_session.state_          = ai_light_session_state::awaiting_spec_confirm;
  io_session.failure_reason_ = "";

  orm::sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_spec>().values(l_spec));
  l_sqls.emplace_back(make_session_update(in_sql, io_session));
  co_await in_sql.run_sql(std::move(l_sqls));

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 生成 Spec revision {}（降级级别 {}，参考图 {} 张）",
      io_session.uuid_id_, l_rev, magic_enum::enum_name(l_spec.degrade_level_), l_req.references_.size()
  );
  co_return;
}

/// 第二步：generating 且 Spec 已确认 → 出候选并派发。
///
/// 与第一步同构（一次接缝调用 + 一个事务 + 事务之后的派发/广播），差别只在本分支会写
/// 轮次行、候选行与分布式任务行。不加 try/catch 的理由同上。
boost::asio::awaitable<void> advance_session_generating_branch(
    sqlite_database& in_sql, ai_light_session& io_session, const uuid& in_operator
) {
  auto l_spec = in_sql.get_current_ai_light_spec(io_session.uuid_id_);
  DOODLE_CHICK_HTTP(l_spec.has_value(), bad_request, "会话 {} 还没有 Spec，无法生成候选", io_session.uuid_id_);
  DOODLE_CHICK_HTTP(
      l_spec->human_confirmed_, bad_request, "会话 {} 的 Spec revision {} 尚未经人工确认（闸门 1），不允许生成候选",
      io_session.uuid_id_, l_spec->revision_
  );

  const auto l_iteration = io_session.current_iteration_ + 1;

  DOODLE_CHICK_HTTP(
      !in_operator.is_nil(), bad_request,
      "会话 {} 无法确定触发人（无 operator_id 且会话无 submitter），无法解析 AI 工作室与计费主体", io_session.uuid_id_
  );

  // iteration 的 uuid 在这里就铸造并随请求交给接缝：那条推理任务行（sd2::task）用
  // ai_light_iteration_id_ 指回来，于是「哪一次推理生成了第 N 轮候选」可以反查。
  // 落库时用的是同一个值，所以任务行、迭代行、候选行三者必然一致。
  const auto l_iteration_id = core_set::get_set().get_uuid();

  candidate_generation_request_t l_req{};
  l_req.session_id_    = io_session.uuid_id_;
  l_req.project_id_    = io_session.project_id_;
  l_req.operator_id_   = in_operator;
  l_req.iteration_id_  = l_iteration_id;
  l_req.iteration_     = l_iteration;
  l_req.spec_          = l_spec->to_spec();
  l_req.presets_       = in_sql.get_ai_light_presets(true);
  l_req.prompt_text_   = io_session.prompt_text_;

  // 上一轮的迭代行只查一次，同时喂给下面两件事。
  const auto l_prev_it = in_sql.get_ai_light_iteration(io_session.uuid_id_, io_session.current_iteration_);

  // warm start：上一轮被选中的候选。首轮没有，由接缝从预设库起手。
  //
  // 必须同时要求「上一轮解的 Spec 就是当前这份」：Spec 升版之后（美术重出了方案）
  // 上一轮的 rig 是照着旧目标打的，拿它当起点会让新目标从一开始就被旧方向带偏。
  if (l_prev_it.has_value() && l_prev_it->spec_id_ == l_spec->uuid_id_) {
    if (auto l_base = in_sql.get_ai_light_selected_candidate(io_session.uuid_id_, io_session.current_iteration_);
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

  // 本轮渲染的基线关卡：直接复用镜头装配的现成推导。
  //
  // §15.8 的决定 D8 原本「只透传不解析」，理由是取它要复刻 shot_render_light_builder 的
  // 三段私有推导（基英/集数 → 场景 UE 路径 → 地编预调探测）。那段推导后来被抽成了自由函数
  // auto_task::shot_render_light()，它自己查库、自己推导，正是这条路线的唯一权威 ——
  // 所以这里改为直接调它，而不是再写一遍（§15.12 批 2）。
  //
  // 会话的 task_id_ 是灯光任务 id，与动画/解算任务指向同一个镜头实体，
  // 所以拿到的就是本镜头的场景关卡、UE 工程与相机文件。
  //
  // 推导失败（缺 entity_shot_extend / frame_in / frame_out 等）**不让整轮失败**：
  // 候选本身仍有价值，空基线由工作机在回传里报错（美术看到的是渲染失败，
  // 而不是「AI 服务出错」）。
  import_and_render_ue_ns::run_ue_assembly_arg l_assembly{};
  bool l_assembly_ok = false;
  if (!io_session.task_id_.is_nil()) {
    try {
      l_assembly    = http::auto_task::shot_render_light(io_session.project_id_, io_session.task_id_);
      l_assembly_ok = true;
    } catch (const std::exception& in_err) {
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮：推导基线关卡失败（{}）", io_session.uuid_id_,
          l_iteration, brief_error(in_err)
      );
    }
  }
  if (l_assembly_ok) l_req.baseline_level_ = l_assembly.original_map_.generic_string();

  if (l_req.baseline_level_.empty()) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(),
        "AI 灯光会话 {} 第 {} 轮：baseline_level 为空。候选可以生成，"
        "但工作机拿不到起点关卡，本轮渲染必然失败。",
        io_session.uuid_id_, l_iteration
    );
  }

  auto l_res = co_await generate_candidates(in_sql, l_req);

  // §6.6 护栏：候选数量与「灯集合一致」是硬约束，不能靠实现方自觉。
  // 数量在这里校验；灯集合一致性需要基线 rig 才谈得上，留到 worker 侧与
  // 候选落库后的回归检查里做（本轮无 worker，先锁住数量这一半）。
  DOODLE_CHICK_HTTP(
      static_cast<std::int32_t>(l_res.rigs_.size()) == g_candidate_count, internal_server_error,
      "候选生成接缝返回了 {} 个候选，方案 §6.6 要求恰好 {} 个", l_res.rigs_.size(), g_candidate_count
  );

  // 产物路径**不是**库里的列（批 7 / D44）：候选行自带 session_id_ / iteration_id_ /
  // uuid_id_，路径由 candidate_render_path() 现算，所以本函数不写任何路径数据。
  // 这里也不再需要 kitsu_ctx_t —— 少一个运行期依赖。

  ai_light_iteration l_it{};
  // 用上面铸造好的那个 uuid，**不要**在这里另取一个：候选的 iteration_id_ 与
  // mark_iteration_rendered 的「候选属于本轮」校验都指向它，而推理任务行
  // （sd2::task.ai_light_iteration_id_）已经用它落了库 —— 再取一个新的就会
  // 让「哪次推理生成本轮」这条链路断开。
  l_it.uuid_id_    = l_iteration_id;
  l_it.session_id_ = io_session.uuid_id_;
  l_it.iteration_  = l_iteration;
  l_it.spec_id_    = l_spec->uuid_id_;
  l_it.state_      = ai_light_iteration_state::generated;
  l_it.set_axes(l_res.axes_);

  // 分布式任务载荷（方案 §13.1 第 5 项：server_task_info.command_ 已有 JSON 列，
  // 不需要改那张表）。形状由 ai_light_task_payload_t 冻结（§15.12 批 0）。
  //
  // 路径分两层（批 6 D38/D39/D40，批 7 D44 收尾）：
  //   * 载荷里只放**相对路径** —— 服务端定命名（out_dir_ = <session>/<轮次 uuid>、
  //     文件名 = <candidate_uuid>.png|.exr），工作机定根（自己的工作机本地缓存根）。
  //     工作机不是共享盘上的进程：它写不了、也不该写服务端 root_ 底下的目录。
  //   * 服务端**不存**路径（D44）：产物上传后落在按同一套命名规则算出来的位置，
  //     要用时现算（candidate_render_path）。所以这里没有任何绝对路径要写库。
  ai_light_task_payload_t l_payload{};
  l_payload.session_id_       = io_session.uuid_id_;
  l_payload.iteration_        = l_iteration;
  l_payload.spec_id_          = l_spec->uuid_id_;
  // 轮次目录名用**轮次 uuid**（批 7）：候选行靠 iteration_id_ 就能算出路径，不必回查序号。
  l_payload.out_dir_          = make_iteration_relative_dir(io_session.uuid_id_, l_it.uuid_id_);
  l_payload.baseline_level_   = l_req.baseline_level_;
  l_payload.ue_project_path_  = l_assembly_ok ? l_assembly.ue_main_project_path_.generic_string() : std::string{};
  l_payload.camera_file_path_ = l_assembly_ok ? l_assembly.camera_file_path_.generic_string() : std::string{};

  orm::sql_modify_statement_vector_t l_sqls{};
  for (std::int32_t l_i = 0; l_i < g_candidate_count; ++l_i) {
    auto l_rig             = l_res.rigs_.at(static_cast<std::size_t>(l_i));
    // 身份字段由服务端补齐：接缝只负责「灯怎么打」，不负责「这是第几个」。
    l_rig.session_id_      = io_session.uuid_id_;
    l_rig.spec_id_         = l_spec->uuid_id_;
    l_rig.iteration_       = l_iteration;
    l_rig.candidate_index_ = l_i;
    l_rig.rig_id_          = uuid{};

    ai_light_candidate l_cand{};
    // uuid 必须在这里就定下来，不能等 ORM 在 insert 时回填：渲染图文件名就是它，
    // nil uuid 会让 6 个候选全部指向同一个路径（先例：build_accepted_preview_file）。
    l_cand.uuid_id_         = core_set::get_set().get_uuid();
    l_cand.iteration_id_    = l_it.uuid_id_;
    l_cand.session_id_      = io_session.uuid_id_;
    l_cand.candidate_index_ = l_i;
    l_cand.set_rig(l_rig);
    l_cand.sync_from_rig();  // 轴三列从 rig 同步过来（DB 列是查询权威）
    l_cand.render_status_ = candidate_render_status::pending;

    l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_candidate>().values(l_cand));
    // 载荷里带的是**相对文件名**（与服务端落库路径的文件名同一个 helper 生成，
    // 只是不带服务端那一长串根）：工作机拼上自己的本地根后交给 UE（D39）。
    l_payload.candidates_.push_back(
        ai_light_task_candidate_t{
            l_i, l_cand.uuid_id_, make_candidate_file_name(l_cand.uuid_id_, false),
            make_candidate_file_name(l_cand.uuid_id_, true), l_rig
        }
    );
  }

  // 任务行。status 保持 submitted，由 computers_assign_task 派发时推进。
  // 「等美术判断」**不是** server_task_info_status —— 那是分布式渲染任务的状态，
  // 两者混用会让工作机的空闲判定错乱（方案 §10.4）。
  server_task_info l_task{};
  l_task.name_   = fmt::format("AI 灯光 {} 轮 {}", io_session.title_.empty() ? "会话" : io_session.title_, l_iteration);
  l_task.status_ = server_task_info_status::submitted;
  l_task.submitter_         = io_session.submitter_;
  l_task.submit_time_       = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_task.task_id_           = io_session.task_id_;
  // 用专门的 ai_light 类型，**不要**复用 auto_light ——
  // auto_light 是既有的「跑 UE 装配」管线（distributed_task.cpp:54），
  // 它的 command_ 形状与本轮完全不同，复用会让工作机按错误的解析方式处理。
  l_task.type_              = server_task_info_type::ai_light;
  // 载荷形状由 ai_light_task_payload_t 冻结（§15.12 批 0），不在这里就地拼 JSON：
  // 工作机与服务端共用同一个 struct 的 to_json/from_json，键名改一处两边同时改。
  l_task.command_           = l_payload;
  // 同上：uuid 要在被引用之前定下来。mark_iteration_rendered() 是**按任务 uuid**
  // 反查轮次的（get_ai_light_iteration_by_task），这里若读到 nil，
  // 轮次的 server_task_info_id_ 就落库为 nil，渲染回传必然 404。
  l_task.uuid_id_           = core_set::get_set().get_uuid();
  l_it.server_task_info_id_ = l_task.uuid_id_;
  l_it.dispatch_at_         = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

  io_session.current_iteration_ = l_iteration;
  io_session.failure_reason_    = "";
  // 状态**留在 generating**：候选已经生成、任务已经派出，但渲染还没回来。
  // 转 awaiting_judgement 由 mark_iteration_rendered() 负责（§10.3）。

  l_sqls.emplace_back(orm::insert(in_sql).into<server_task_info>().values(l_task));
  l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
  l_sqls.emplace_back(make_session_update(in_sql, io_session));
  co_await in_sql.run_sql(std::move(l_sqls));

  // 派发 + 广播。放在事务之后：任务行已经落库，派发失败也只是「暂时没人领」，
  // computers_assign_task 下次轮询会再捡起来，不会丢。
  co_await http::computers_assign_task::get_instance().run_next_task();
  socket_io::broadcast(socket_io::server_task_info_new_broadcast_t{.server_task_info_id_ = l_task.uuid_id_});

  SPDLOG_LOGGER_INFO(
      g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮 {} 个候选已派发（任务 {}）", io_session.uuid_id_,
      l_iteration, g_candidate_count, l_task.uuid_id_
  );
  co_return;
}

}  // namespace

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
  /// 会话已经**没有终态**了（§15.10），所以这里退回原状态不是为了「让它可恢复」——
  /// 而是为了不留一个卡死的 generating：接缝超时、模型输出不合规、解析不出工作室
  /// 这类失败都是可重试的，退回原状态后美术直接再点一次即可，且不丢已经上传的参考图
  /// （参考图挂在会话上，`cascade`）。失败的唯一记录是 `failure_reason_`。
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
      co_await advance_session_spec_branch(in_sql, in_session, l_operator);
      co_return;
    }

    if (l_entry_state == ai_light_session_state::generating) {
      co_await advance_session_generating_branch(in_sql, in_session, l_operator);
      co_return;
    }

    // ---------------------------------------------------------------
    // 其它状态：没有可推进的步骤
    // ---------------------------------------------------------------
    DOODLE_CHICK_HTTP(
        false, bad_request, "会话 {} 处于状态 {}，当前没有可推进的步骤", in_session.uuid_id_,
        magic_enum::enum_name(l_entry_state)
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
  // 会话**没有终态**了，所以这里不再有分支。generating 是唯一能再出一轮的状态
  // （`advance_session` 的 generating 分支 = `POST /iterate`），而一个轮次判断完之后，
  // 唯一的前进方向就是再出一轮 —— 美术对同一个镜头的灯光可以无限轮调整。
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
    // 定稿原先刻意不置 is_selected_，理由是「定稿没有下一轮」—— 会话没有终态之后
    // 这个理由不成立了：定稿之后接着调时，下一轮的 warm start
    // （get_ai_light_selected_candidate）应该从**定稿那套 rig** 出发（那是目前最好的一张），
    // 而不是回到 LLM 的原始基线。
    //
    // is_selected_ 与 is_accepted_ 仍然是两个语义（前者 = 下一轮基准，后者 = 当前定稿），
    // 只是现在可以在同一行上同时为真。
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

boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const std::vector<ai_light_candidate>& in_candidates
) {
  auto l_it = in_sql.get_ai_light_iteration_by_task(in_server_task_info_id);
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
    // 不写路径（批 7 / D44）：库里没有那两列了，要路径时按 candidate_render_path() 现算
    // （上传路由、取图路由、apply_render_report 的 D41/D42 都这么做）。
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
  l_ret.error_            = json_val::opt_string(in_json, "error").value_or("");

  const auto l_candidates = json_val::require_array(in_json, g_render_report_seam, "candidates");
  l_ret.candidates_.reserve(l_candidates.size());

  for (std::size_t i = 0; i < l_candidates.size(); ++i) {
    const auto l_base  = fmt::format("candidates[{}]", i);

    // 数组元素必须是对象。带上下标路径再报错，比「解析失败」四个字有用得多。
    const auto* l_item = json_val::json_at_path(in_json, l_base);
    DOODLE_CHICK_HTTP(
        l_item != nullptr && l_item->is_object(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望对象）",
        g_render_report_seam, l_base
    );

    candidate_render_report_t l_cand{};

    // candidate_id 是严格字段：它对不上任何既存候选时整轮回传都没意义（apply_render_report 会拒）。
    //
    // 手写「取 + 转」的理由：校验层**没有** uuid 读取器 —— 模型输出里从不出现 uuid
    // （候选身份一律由服务端钉），所以当初没有造。这里把 nlohmann 的解析异常翻成
    // 同一套 bad_gateway 措辞，好让「回传坏了」在美术那边只有一种说法。
    const auto l_id_path = l_base + ".candidate_id";
    const auto* l_id     = json_val::json_at_path(in_json, l_id_path);
    DOODLE_CHICK_HTTP(
        l_id != nullptr && l_id->is_string(), bad_gateway, "{}：字段 {} 缺失或类型不符合（期望 uuid 字符串）",
        g_render_report_seam, l_id_path
    );
    try {
      l_cand.candidate_id_ = l_id->get<uuid>();
    } catch (const std::exception& in_err) {
      DOODLE_CHICK_HTTP(
          false, bad_gateway, "{}：字段 {} 不是合法的 uuid（{}）", g_render_report_seam, l_id_path, in_err.what()
      );
    }

    l_cand.status_ = json_val::require_enum<candidate_render_status>(in_json, g_render_report_seam, l_base + ".status");

    // 以下四个是宽松字段（D26：缺失取默认、形状不符丢弃）：
    //   * candidate_index 缺失时按下标推 —— 工作机的回传顺序就是派发顺序；
    //   * render_ms / light_count 缺失记 0。这不会把失败判成成功：判据只有 status_；
    //   * metrics 是诊断数据（给美术看落点、给 VLM 评分用），形状错就丢，不让它把
    //     6 张已经渲出来的图作废。
    l_cand.candidate_index_ =
        static_cast<std::int32_t>(json_val::opt_number(*l_item, "candidate_index").value_or(static_cast<double>(i)));
    l_cand.render_ms_   = static_cast<std::int64_t>(json_val::opt_number(*l_item, "render_ms").value_or(0.0));
    l_cand.light_count_ = static_cast<std::int32_t>(json_val::opt_number(*l_item, "light_count").value_or(0.0));

    if (l_item->contains("metrics")) {
      try {
        l_item->at("metrics").get_to(l_cand.metrics_);
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
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const std::string& in_reason
) {
  auto l_it = in_sql.get_ai_light_iteration_by_task(in_server_task_info_id);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);

  auto l_session            = in_sql.get_by_uuid<ai_light_session>(l_it->session_id_);
  l_session.failure_reason_ = brief_text(in_reason);

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
    const round_render_report_t& in_report
) {
  auto l_it = in_sql.get_ai_light_iteration_by_task(in_server_task_info_id);
  DOODLE_CHICK_HTTP(l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id);

  // 整轮失败由 error_ 表达（UE 起不来 / 超时 / 载荷非法）；「一张 rendered 都没有」也算失败 ——
  // 否则轮次会被推到 awaiting_judgement，美术面对 6 张不存在的图点「选第 3 张」。
  // 注意：这只是**回报文里**的初判。文件是否真的存在在循环里逐张查（D41），
  // 查完还要再判一次（l_any_rendered_after），否则「6 张都声称 rendered 但文件都不在」
  // 会绕过这里的拦截，把空轮次推给美术。
  if (!in_report.error_.empty() ||
      !std::any_of(
          in_report.candidates_.begin(), in_report.candidates_.end(),
          [](const candidate_render_report_t& in_cand) { return in_cand.status_ == candidate_render_status::rendered; }
      )) {
    co_await mark_iteration_failed(
        in_sql, in_server_task_info_id,
        !in_report.error_.empty()
            ? in_report.error_
            : fmt::format("轮次 {} 的 {} 个候选全部渲染失败", l_it->iteration_, in_report.candidates_.size())
    );
    co_return;
  }

  auto l_round = in_sql.get_ai_light_candidates(l_it->uuid_id_);

  std::vector<ai_light_candidate> l_candidates{};
  l_candidates.reserve(in_report.candidates_.size());
  std::size_t l_missing_png = 0;

  for (const auto& l_rep : in_report.candidates_) {
    // 候选必须属于**本轮**（只查本轮，不查全库）：别的会话的候选 uuid 混进来必须拒掉，
    // 否则一个坏回传就能改到别人的候选行上。
    const auto l_found = std::find_if(l_round.begin(), l_round.end(), [&](const ai_light_candidate& in_cand) {
      return in_cand.uuid_id_ == l_rep.candidate_id_;
    });
    DOODLE_CHICK_HTTP(
        l_found != l_round.end(), bad_request, "候选 {} 不属于 AI 灯光轮次 {}", l_rep.candidate_id_, l_it->iteration_
    );

    // 产物路径既不在回传里（D27），也不在库里（批 7 / D44）：它就是候选行那三个 uuid 的
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

    // D41：声称 rendered 就要能取到图。工作机的上传是**独立一步**（D40），
    // 完全可能出现「UE 写了文件、报告说 rendered、上传那一步失败了」——
    // 这时如果照样落 rendered，前端会拿到一个 404 的候选图，而美术的判断通道只能靠这张图。
    // 降级成 failed 是**保守**选择：本轮其余候选照常进入判断，只是这张不可选。
    if (l_cand.render_status_ == candidate_render_status::rendered) {
      const FSys::path l_png = candidate_render_path(in_ctx, l_cand, false);
      if (!FSys::exists(l_png)) {
        ++l_missing_png;
        l_cand.render_status_ = candidate_render_status::failed;
        SPDLOG_LOGGER_WARN(
            g_logger_ctrl().get_http(), "AI 灯光回传：候选 {}（轮次 {}）声称渲染成功，但产物不在 {}，已降级为失败",
            l_cand.uuid_id_, l_it->iteration_, l_png.generic_string()
        );
      }
    }

    // EXR 是**可选**产物（D42：工作机传不上去不阻断本轮），所以这里不降级。
    // 批 7 起库里没有 EXR 路径可清了（D44），只留一条诊断：算出来的 EXR 不在时记一笔，
    // 让「这一轮的线性图没上来」在日志里看得见。
    // 「留了一条指向不存在文件的陈旧记录」那类问题随那两列一起消失了 —— 路径是现算的。
    const FSys::path l_exr = candidate_render_path(in_ctx, l_cand, true);
    if (!FSys::exists(l_exr)) {
      SPDLOG_LOGGER_WARN(
          g_logger_ctrl().get_http(),
          "AI 灯光回传：候选 {}（轮次 {}）的线性 EXR 不在 {}（可选产物，不影响本轮判断）", l_cand.uuid_id_,
          l_it->iteration_, l_exr.generic_string()
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
        in_sql, in_server_task_info_id,
        fmt::format("轮次 {} 的 {} 个候选渲染成功但产物文件缺失，本轮无可判断结果", l_it->iteration_, l_missing_png)
    );
    co_return;
  }

  // 落库与状态流转全部复用原有那一份实现：两条入口（单测 / worker 回传）走同一段事务，
  // 免得「回传路径」与「测试路径」慢慢长成两套语义。
  co_await mark_iteration_rendered(in_sql, in_server_task_info_id, l_candidates);
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
  l_detail.spec_history_         = in_sql.get_ai_light_specs(in_session_id);
  l_detail.current_spec_         = in_sql.get_current_ai_light_spec(in_session_id);
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