//
// AI 灯光：轮次编排与状态机实现
//
// 对应 docs/ue_ai_lighting_plan.md §10.3（状态流转）与 §10.4（派发契约）。
//
// 本文件只装**状态机**：advance_session / apply_judgement / mark_iteration_rendered /
// build_session_detail，以及它自己的两个小助手。三条「接缝」（generate_spec /
// generate_candidates / structure_feedback）、rig 纯运算、共用部件与 JSON 架构层在同目录的
// 独立文件里：
//   * `ai_light_seam_spec.{h,cpp}`        —— 蒸馏 Spec（§15.7）
//   * `ai_light_seam_candidates.{h,cpp}`  —— 基线 rig + 轴计划 + OFAT 展开（§15.8）
//   * `ai_light_seam_feedback.{h,cpp}`    —— 自由文本 → 轴方向（§15.9）
//   * `ai_light_rig_ops.{h,cpp}`          —— 轴施加 / OFAT 展开 / ops diff / key:fill 比
//   * `ai_light_seam_common.{h,cpp}`      —— 接缝共用提示词片段 + 描述 + 推理管道
//   * `ai_light_json_validation.{h,cpp}`  —— schema 建造器 + 严格/宽松读取器 + 架构诊断
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
/// 独立成函数：这一段本身就是「一次接缝调用 + 一个事务」的完整闭环，与下面出候选那一段
/// 没有共享状态 —— 唯一的共享是 `io_session`（状态与 `failure_reason_` 由本函数直接改，
/// 因为回滚要用到它们）。
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

/// 第二步：generating 且 Spec 已确认 → 出候选，然后按 in_local_only 分两条出口：
///
///   * `false`（默认，队列路线）：建 server_task_info{type = ai_light} → run_next_task + 广播，
///     由工作机起 UE 渲染（§10.4）；
///   * `true`（引擎内路线，D52/D53）：**不建任务行、不派发、不广播**，把「本轮该怎么渲」
///     填进返回值交给调用方（发起方本机自己渲染，见 ai_light_launch_plan_t 的说明）。
///
/// 两条出口共用前面全部内容（Spec 校验、warm start、接缝调用、轮次行、候选行、载荷装配）——
/// 差别只在末尾那一段，所以用一个分支参数而不是写两个函数：复制一遍的话，
/// 上面那几十行校验会慢慢长成两套语义。
///
/// 不加 try/catch 的理由同第一步。
boost::asio::awaitable<std::optional<ai_light_launch_plan_t>> advance_session_generating_branch(
    sqlite_database& in_sql, ai_light_session& io_session, const uuid& in_operator, bool in_local_only
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

  // 本轮渲染的对象是 auto_light 装配出来的交付场景：关卡（render_map_）里有场景，
  // 序列（level_sequence_import_）里有相机轨道、camera cut 与角色动画，配置资产
  // （movie_pipeline_config_）里有这条镜头的 MRQ 画质设置。三者都由 shot_render_light()
  // 现算 —— 它就是那三段推导（基英/集数 → 场景 UE 路径 → 地编预调探测）的唯一权威，
  // 自己查库、自己推导，所以这里直接调它，不另写一遍。
  //
  // 会话的 task_id_ 是灯光任务 id，与动画/解算任务指向同一个镜头实体。
  //
  // 装配推导失败 / 序列为空 = 本轮不可渲染（D50）：候选本身虽然生成了，但工作机拿着一个
  // 没有序列的载荷去起 UE，只会失败在更远的地方、现象与真正的原因无关。所以抛在这里，
  // 由 advance_session 回滚会话状态并把原因写进 failure_reason_。
  // 用 _with_sync 变体：装配参数之外还要拿到「工作机工程根 + 装配文件同步清单」，
  // 队列出口把它们一并下发给工作机（D57：UE 工程是每台机器的本地缓存，共享盘素材
  // 必须先差分拷进去，否则工作机上没有工程可开）。
  http::auto_task::shot_render_light_result l_assembly_res{};
  std::string l_assembly_err{};
  if (io_session.task_id_.is_nil()) {
    l_assembly_err = "会话没有关联镜头任务（task_id 为空）";
  } else {
    try {
      l_assembly_res = http::auto_task::shot_render_light_with_sync(io_session.project_id_, io_session.task_id_);
    } catch (const std::exception& in_err) {
      l_assembly_err = brief_error(in_err);
    }
  }
  auto&& l_assembly = l_assembly_res.arg_;
  DOODLE_CHICK_HTTP(
      l_assembly_err.empty(), internal_server_error,
      "AI 灯光会话 {} 第 {} 轮：无法推导本镜头的渲染装配，本轮无法渲染（{}）", io_session.uuid_id_, l_iteration,
      l_assembly_err
  );
  DOODLE_CHICK_HTTP(
      !l_assembly.level_sequence_import_.empty(), internal_server_error,
      "AI 灯光会话 {} 第 {} 轮：镜头装配没有给出 level sequence（{}），本轮无法渲染", io_session.uuid_id_,
      l_iteration, l_assembly.level_sequence_import_.generic_string()
  );
  l_req.baseline_level_ = l_assembly.original_map_.generic_string();

  auto l_res = co_await generate_candidates(in_sql, l_req);

  // §6.6 护栏：候选数量与「灯集合一致」是硬约束，不能靠实现方自觉。
  // 数量在这里校验；灯集合一致性需要基线 rig 才谈得上，留到 worker 侧与
  // 候选落库后的回归检查里做（本轮无 worker，先锁住数量这一半）。
  DOODLE_CHICK_HTTP(
      static_cast<std::int32_t>(l_res.rigs_.size()) == g_candidate_count, internal_server_error,
      "候选生成接缝返回了 {} 个候选，方案 §6.6 要求恰好 {} 个", l_res.rigs_.size(), g_candidate_count
  );

  // 产物落点**不是**库里的列（D44）：候选行自带 session_id_ / iteration_id_ / uuid_id_，
  // 路径由 candidate_render_path() 现算，所以本函数不写任何路径数据，也不需要 kitsu_ctx_t。

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
  // 不需要改那张表）。形状由 ai_light_task_payload_t 冻结。
  //
  // 这一轮渲染的是 auto_light 那条交付链路现成的场景：render_map_ 提供关卡，
  // level_sequence_ 提供相机轨道、camera cut 与角色动画，movie_pipeline_config_ 提供
  // 这条镜头的 MRQ 画质设置 —— 灯光候选只换灯，镜头运动与表演照原样（D45）。
  // 三者都是包路径，UE 侧直接当 FSoftObjectPath / 命令行地图参数用。
  //
  // 路径分两层：
  //   * 载荷里只放**相对路径** —— 服务端定命名（out_dir_ = <session uuid>/<轮次 uuid>；
  //     候选文件名 = <candidate_uuid>.png|.exr、<candidate_uuid>.mp4、<candidate_uuid>/frame_*），
  //     工作机定根（自己的工作机本地缓存根）。工作机不是共享盘上的进程：
  //     它写不了、也不该写服务端 root_ 底下的目录。
  //   * 服务端**不存**路径（D44）：产物上传后落在按同一套命名规则算出来的位置，
  //     要用时现算（candidate_render_path / candidate_movie_path）。所以这里没有任何绝对路径要写库。
  // 两条出口共用的装配（队列载荷 / 本地渲染计划）：形状逐字一致的渲染段。
  // 用 lambda 生成「带相对路径的候选」而不是写两遍，是因为这段正是一轮渲染的
  // wire 契约所在 —— 两遍迟早会漂移，而漂移的表现是「本地路线算不出视频路径」这类
  // 只在真机上才发现的故障。
  auto l_out_dir = make_iteration_relative_dir(io_session.uuid_id_, l_it.uuid_id_);

  orm::sql_modify_statement_vector_t l_sqls{};
  std::vector<ai_light_task_candidate_t> l_payload_candidates{};
  l_payload_candidates.reserve(static_cast<std::size_t>(g_candidate_count));
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
    // 载荷里带的是**相对路径**（由 make_* 系列 helper 生成，与服务端算落点时用的是同一套
    // 命名规则，只是不带服务端那一长串根）：工作机拼上自己的本地根后交给 UE（D39）。
    // 每候选三样产物（D47）：MRQ 逐帧目录（frames_dir_，UE 往这里写 frame_*.png）、
    // 视频（movie_path_，工作机用 OpenCV 合成）、首帧海报（png_path_，工作机从帧目录复制）。
    // exr_path_ 本轮不产，保留在载荷里是为了线性交付那条线（D42）。
    l_payload_candidates.push_back(
        ai_light_task_candidate_t{
            l_i, l_cand.uuid_id_, make_candidate_frames_dir(l_cand.uuid_id_),
            make_candidate_file_name(l_cand.uuid_id_, false), make_candidate_file_name(l_cand.uuid_id_, true),
            make_candidate_movie_file(l_cand.uuid_id_), l_rig
        }
    );
  }

  io_session.current_iteration_ = l_iteration;
  io_session.failure_reason_    = "";
  // 状态**留在 generating**：候选已经生成、渲染还没回来。
  // 转 awaiting_judgement 由 mark_iteration_rendered() 负责（§10.3），两条路线共用。

  // ---------------------------------------------------------------------
  // 引擎内路线（D52/D53）：不建任务行、不派发、不广播，把渲染计划原样返回。
  // ---------------------------------------------------------------------
  if (in_local_only) {
    ai_light_launch_plan_t l_plan{};
    l_plan.session_id_            = io_session.uuid_id_;
    l_plan.iteration_id_          = l_it.uuid_id_;
    l_plan.iteration_             = l_iteration;
    l_plan.out_dir_               = l_out_dir;
    l_plan.render_map_            = l_assembly.render_map_.generic_string();
    l_plan.level_sequence_        = l_assembly.level_sequence_import_.generic_string();
    l_plan.movie_pipeline_config_ = l_assembly.movie_pipeline_config_.generic_string();
    l_plan.frame_begin_           = l_assembly.begin_time_;
    l_plan.frame_end_             = l_assembly.end_time_;
    l_plan.ue_project_path_       = l_assembly.ue_main_project_path_.generic_string();
    l_plan.camera_file_path_      = l_assembly.camera_file_path_.generic_string();
    // D57：引擎内路线由发起方本机渲染，装配文件同样要同步到**本机**缓存。
    // 本机就是发起方，路径与清单一起带走，由客户端在起 UE 之前差分拷贝。
    l_plan.project_dir_           = l_assembly_res.project_dir_;
    l_plan.assembly_files_        = std::move(l_assembly_res.assembly_files_);
    l_plan.candidates_            = std::move(l_payload_candidates);

    // 轮次行的 server_task_info_id_ 留 nil：这条路线**没有**任务行，
    // 回传因此不能按任务号找轮次，改按会话 + 轮次号定位（D54 的 ai_light_iteration_locator_t）。
    l_sqls.emplace_back(orm::insert(in_sql).into<ai_light_iteration>().values(l_it));
    l_sqls.emplace_back(make_session_update(in_sql, io_session));
    co_await in_sql.run_sql(std::move(l_sqls));

    SPDLOG_LOGGER_INFO(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 第 {} 轮 {} 个候选已生成（引擎内路线，渲染计划已返回，等待发起方本机渲染）",
        io_session.uuid_id_, l_iteration, g_candidate_count
    );
    co_return std::optional<ai_light_launch_plan_t>{std::move(l_plan)};
  }

  // ---------------------------------------------------------------------
  // 队列路线（默认）：建任务行 → 派发 → 广播，由工作机起 UE 渲染。
  // ---------------------------------------------------------------------
  ai_light_task_payload_t l_payload{};
  l_payload.session_id_            = io_session.uuid_id_;
  l_payload.iteration_             = l_iteration;
  l_payload.spec_id_               = l_spec->uuid_id_;
  // 轮次目录名用**轮次 uuid**：候选行靠 iteration_id_ 就能算出落点，不必回查序号。
  l_payload.out_dir_               = l_out_dir;
  l_payload.baseline_level_        = l_req.baseline_level_;
  l_payload.render_map_            = l_assembly.render_map_.generic_string();
  l_payload.level_sequence_        = l_assembly.level_sequence_import_.generic_string();
  l_payload.movie_pipeline_config_ = l_assembly.movie_pipeline_config_.generic_string();
  // 帧区间只作诊断：真正渲染哪一段由序列自己的 playback range 决定（D48）。
  l_payload.frame_begin_           = l_assembly.begin_time_;
  l_payload.frame_end_             = l_assembly.end_time_;
  l_payload.ue_project_path_       = l_assembly.ue_main_project_path_.generic_string();
  l_payload.camera_file_path_      = l_assembly.camera_file_path_.generic_string();
  // D57：工作机上没有这份 UE 工程缓存，装配素材必须下发同步清单。
  // ue_project_path_ 仍是服务端路径（保留给日志与老工作机），工作机会按
  // project_dir_ + 本机 user_work_root_ 自算工程根并覆盖它。
  l_payload.project_dir_           = l_assembly_res.project_dir_;
  l_payload.assembly_files_        = std::move(l_assembly_res.assembly_files_);
  l_payload.candidates_            = std::move(l_payload_candidates);

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
  // 载荷形状由 ai_light_task_payload_t 冻结，不在这里就地拼 JSON：
  // 工作机与服务端共用同一个 struct 的 to_json/from_json，键名改一处两边同时改。
  l_task.command_           = l_payload;
  // 同上：uuid 要在被引用之前定下来。mark_iteration_rendered() 是**按任务 uuid**
  // 反查轮次的（get_ai_light_iteration_by_task），这里若读到 nil，
  // 轮次的 server_task_info_id_ 就落库为 nil，渲染回传必然 404。
  l_task.uuid_id_           = core_set::get_set().get_uuid();
  l_it.server_task_info_id_ = l_task.uuid_id_;
  l_it.dispatch_at_         = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};

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
  co_return std::optional<ai_light_launch_plan_t>{};
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
  /// 会话**没有终态**（§15.10），所以这里退回原状态不是为了「让它可恢复」——
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
      // 队列路线：建任务行 + 派发（忽略返回的渲染计划 —— 它只给引擎内路线用）。
      co_await advance_session_generating_branch(in_sql, in_session, l_operator, false);
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

boost::asio::awaitable<std::optional<ai_light_launch_plan_t>> advance_session_local(
    sqlite_database& in_sql, ai_light_session in_session, uuid in_operator_id
) {
  const auto l_entry_state = in_session.state_;

  // 与 advance_session 逐字一致的前置校验：参考图为空直接 400，且**不**写 failure_reason_。
  // 两条路线共用同一套准入条件是有意的 —— 「引擎内模式可以少传一张参考图」这种偏差
  // 会让同一个会话在两种模式下蒸出不同的 Spec。
  if (l_entry_state == ai_light_session_state::draft ||
      l_entry_state == ai_light_session_state::awaiting_spec_confirm) {
    DOODLE_CHICK_HTTP(
        !in_sql.get_ai_light_references(in_session.uuid_id_).empty(), bad_request,
        "会话 {} 还没有参考图，无法蒸馏 Spec（决策 1：参考必有多张）", in_session.uuid_id_
    );
  }

  // 回滚 lambda：与 advance_session 里那份语义相同（退回进入状态 + 记 failure_reason_）。
  // 单独写一份而不是抽成共用函数，是因为它按引用捕获这个栈上的 in_session，
  // 抽出去就得把 in_session 与 entry_state 一起当参数传，反而更难读。
  auto l_rollback = [&](const std::string& in_reason) -> boost::asio::awaitable<void> {
    in_session.state_          = l_entry_state;
    in_session.failure_reason_ = in_reason;
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "AI 灯光会话 {} 推进失败（引擎内路线，停留在 {}）: {}", in_session.uuid_id_,
        magic_enum::enum_name(l_entry_state), in_reason
    );
    try {
      orm::sql_modify_statement_vector_t l_sqls{};
      l_sqls.emplace_back(make_session_update(in_sql, in_session));
      co_await in_sql.run_sql(std::move(l_sqls));
    } catch (const std::exception& in_err) {
      SPDLOG_LOGGER_ERROR(
          g_logger_ctrl().get_main_error(), "AI 灯光会话 {} 回滚失败: {}", in_session.uuid_id_, in_err.what()
      );
    }
  };

  std::optional<ai_light_launch_plan_t> l_plan{};
  std::exception_ptr l_err{};
  std::string l_reason{"未知错误"};

  try {
    const auto l_operator = in_operator_id.is_nil() ? in_session.submitter_ : in_operator_id;

    if (l_entry_state == ai_light_session_state::draft ||
        l_entry_state == ai_light_session_state::awaiting_spec_confirm) {
      // 这一步没有渲染计划可给：Spec 出完会话进 awaiting_spec_confirm（闸门 1），
      // 候选要等人工确认之后再走一次调用。返回 nullopt 让调用方照常回详情。
      co_await advance_session_spec_branch(in_sql, in_session, l_operator);
      co_return l_plan;
    }

    if (l_entry_state == ai_light_session_state::generating) {
      l_plan = co_await advance_session_generating_branch(in_sql, in_session, l_operator, true);
      co_return l_plan;
    }

    DOODLE_CHICK_HTTP(
        false, bad_request, "会话 {} 处于状态 {}，当前没有可推进的步骤", in_session.uuid_id_,
        magic_enum::enum_name(l_entry_state)
    );
  } catch (...) {
    // 同 advance_session：catch 里不能 co_await，只收集。
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
  co_return l_plan;
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
  DOODLE_CHICK_HTTP(
      l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id
  );
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
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::string& in_reason
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(
      l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id
  );

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
    const ai_light_iteration_locator_t& in_locator, const round_render_report_t& in_report
) {
  auto l_it = locate_iteration(in_sql, in_server_task_info_id, in_locator);
  DOODLE_CHICK_HTTP(
      l_it.has_value(), not_found, "没有任务 {} 对应的 AI 灯光轮次", in_server_task_info_id
  );

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
      const FSys::path l_png       = candidate_render_path(in_ctx, l_cand, false);
      const FSys::path l_movie     = candidate_movie_path(in_ctx, l_cand);
      const bool l_png_exists      = FSys::exists(l_png);
      const bool l_movie_exists    = FSys::exists(l_movie);
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