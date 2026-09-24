//
// AI 灯光：轮次编排与状态机（docs/ue_ai_lighting_plan.md §10.3 / §13.1）
//
// 这一层刻意**不包含任何 HTTP 依赖**：
//   * 单测可以直接调状态机，不必拉起 session_data / JWT；
//   * 测试二进制不会被 http 头污染。
// 路由层（http_method/kitsu/ai_light.h）只是这一层的薄包装。
//
// ---------------------------------------------------------------------------
// 三个「接缝」：本轮有意留空的部分
// ---------------------------------------------------------------------------
// generate_spec / generate_candidates / structure_feedback 需要 deepseek_client
// （方案 §13.1 第 4 项）与参考图统计（第 8 项），两者都还没实现，所以它们统一抛
// `doodle_error{status::not_implemented}`，由 http_session_data.cpp:149-151 映射成 501。
//
// 签名先定下来，下一里程碑只改这三个函数的函数体，路由层与状态机不动。
//
// 接缝契约（实现方必须遵守）：
//   1. 抛出即视为「本轮无法推进」。调用方 advance_session() 会把会话状态回滚到进入时的
//      状态并记录 failure_reason_，所以实现方**不需要**自己清理状态。
//   2. 但必须保证「抛之前没有落库」，或者「落库的部分是幂等可重入的」——
//      调用方只回滚会话状态这一列，不会回滚接缝自己写下的东西。
//   3. 返回值必须自洽：候选数量 == g_candidate_count，且满足 §6.6 护栏
//      （同轮候选的灯集合与角色完全一致，只有轴参数不同）。调用方会再校验一次，
//      不合规直接判失败 —— 护栏是硬约束，不能靠实现方自觉。
//

#pragma once

#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/view.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/asio/awaitable.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

// ===========================================================================
// 接缝 1：提示词（+ 参考图统计）→ LightingSpec
// ===========================================================================

struct DOODLELIB_API spec_generation_request_t {
  uuid session_id_{};
  /// 人工提示词原文（权威意图）。包含氛围、角色性格、故事上下文、背景设定。
  std::string prompt_text_{};
  /// 已上传的参考图。本轮它们的 stats_ 恒为空（统计未实现），
  /// 所以实际能用的只有 file_path_ / width_ / height_。
  std::vector<ai_light_reference> references_{};
  /// 参考图统计缺失时只能是 prompt_only。方案 §4.2 本就定义了这两个降级级别，
  /// 所以「统计没落地」不是临时凑合，而是走既有降级路径。
  spec_degrade_level degrade_level_{spec_degrade_level::prompt_only};
  /// 重出方案时的上一版（美术对上一版不满意 → 重新蒸馏）
  std::optional<lighting_spec_t> previous_spec_{};
};

struct DOODLELIB_API spec_generation_result_t {
  lighting_spec_t spec_{};
  /// 键为字段路径（如 "targets.contrast"），与 ai_light_spec::evidence_ 同形状 ——
  /// evidence 的语义就是「这个字段是从哪来的、有多可信」，没有键就失去意义。
  std::map<std::string, evidence_entry_t> evidence_{};
  reference_stats_t reference_stats_{};
  conflict_resolution_t conflict_resolution_{};
};

/// 提示词（+ 参考图统计）→ LightingSpec。**本轮抛 501。**
DOODLELIB_API boost::asio::awaitable<spec_generation_result_t> generate_spec(
    const spec_generation_request_t& in_request
);

// ===========================================================================
// 接缝 2：基线 rig + Spec → 本轮 6 个候选
// ===========================================================================

struct DOODLELIB_API candidate_generation_request_t {
  uuid session_id_{};
  /// 从 1 开始
  std::int32_t iteration_{1};
  lighting_spec_t spec_{};
  /// warm start 起点 = 上一轮被选中的候选；首轮为空（从预设库起手）
  std::optional<lighting_rig_t> base_rig_{};
  /// 上一轮人工判断指定的轴。滑块通道（默认）直接给数值，不经过 LLM。
  std::optional<iteration_axes_t> feedback_axes_{};
  /// 候选的预设库，供检索起手
  std::vector<ai_light_preset> presets_{};
};

struct DOODLELIB_API candidate_generation_result_t {
  /// 必须恰好 g_candidate_count 个
  std::vector<lighting_rig_t> rigs_{};
  iteration_axes_t axes_{};
};

/// 基线 rig + Spec → 本轮候选。**本轮抛 501。**
DOODLELIB_API boost::asio::awaitable<candidate_generation_result_t> generate_candidates(
    const candidate_generation_request_t& in_request
);

// ===========================================================================
// 接缝 3：自由文本反馈 → 结构化轴调整
// ===========================================================================

/// 把「主光再低一点，色温再冷一点」这类自由文本转成轴调整（方案 §13.1 第 9 项）。
///
/// **滑块路径不走这里**（决策 5：滑块为默认），所以缺了它 /judge 依然完全可用 ——
/// 调用方捕获 not_implemented 后降级为「文本原样留存、轴为空」，判断照常成功。
/// **本轮抛 501。**
DOODLELIB_API boost::asio::awaitable<std::vector<axis_step_t>> structure_feedback(
    const uuid& in_session_id, const std::string& in_feedback_text
);

// ===========================================================================
// 状态机
// ===========================================================================

/// 会话状态推进的唯一入口。按 in_session.state_ 分派：
///
///   draft / suspended 且尚无 Spec
///       → generate_spec 接缝 → 写 ai_light_spec(revision = max+1)
///       → session = awaiting_spec_confirm
///   generating 且已有已确认的 Spec
///       → generate_candidates 接缝 → 写 iteration + g_candidate_count 个 candidate
///       → 建 server_task_info{type = ai_light} → run_next_task() + broadcast
///       → session 保持 generating（渲染中）；渲染完由 mark_iteration_rendered 转 awaiting_judgement
///
/// 失败时回滚会话状态到进入时的状态、记 failure_reason_，然后重新抛出。
/// 这样不会留下卡在 generating 的会话，也不会丢已上传的参考图
/// （参考图的外键 cascade 挂在会话上，删会话才会连带删图）。
DOODLELIB_API boost::asio::awaitable<void> advance_session(sqlite_database& in_sql, ai_light_session in_session);

/// 人工闸门 2 的状态推进。**纯 DB，不碰文件与 LLM** —— 单测直接调它。
///
/// 前置条件（由调用方校验）：
///   * in_iteration 就是 in_judgement 所判断的那一轮；
///   * in_judgement.selected_candidate_id_ 属于 in_iteration（防跨轮误选）；
///   * decision == accept 时 in_accepted_preview_file 必须已填好（id / 尺寸 / 路径），
///     其余 decision 必须为空。
///
/// 为什么 preview_file 由调用方构造、却由本函数插入：定稿要同时写三处
/// （preview_file 行、candidate 的定稿标记与 preview_file_id_、会话状态），
/// 分两个事务提交就会出现「预览文件已建但候选没标定稿」这种半成品状态。
/// 调用方负责碰文件（OpenCV 读图 + save_variants 落变体），本函数负责一次事务落库。
///
/// preview_file 按值传入而非 const 引用：ORM 的 values() 会回填 uuid_id_ / created_at_
/// 这类服务端生成的字段，必须拿到可写对象。
///
/// 本函数负责：插入 preview_file、标记候选（accept → mark_accepted()；
/// continue → is_selected_ = true）、回填 candidate.preview_file_id_、
/// 写 iteration 的判断记录、推进 session 状态。
DOODLELIB_API boost::asio::awaitable<void> apply_judgement(
    sqlite_database& in_sql, ai_light_session& in_session, ai_light_iteration& in_iteration,
    const ai_light_judgement_t& in_judgement, std::optional<preview_file> in_accepted_preview_file = {}
);

/// worker 回传渲染完成后调用（§10.3 的「落 iteration → 置 awaiting_judgement」）。
///
/// 本轮没有 worker（§13.3 未做），所以这个函数只提供入口并纳入单测；
/// worker 里程碑把回传处理接上即可，状态机本身不用改。
/// in_candidates 是回传的候选（含 render_png_path_ / render_ms_ / light_count_ / metrics_）。
DOODLELIB_API boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const std::vector<ai_light_candidate>& in_candidates
);

/// 挂起扫描器的可测内核：把已过判断时限、仍停在 awaiting_judgement 的会话转成 suspended。
/// 返回被挂起的会话条数。重复调用幂等（第二次返回 0）。
DOODLELIB_API std::int32_t scan_overdue_sessions(sqlite_database& in_sql);

/// 组装会话详情读模型。会话不存在时返回 nullopt。
DOODLELIB_API std::optional<ai_light_session_detail_t> build_session_detail(
    sqlite_database& in_sql, const uuid& in_session_id
);

// ===========================================================================
// 挂起扫描器（常驻后台）
// ===========================================================================

/// 4 小时不判断 → 会话挂起（决策 11）。
///
/// 与 seedance2 的 task_run_manager（seedance2/task.cpp:331-337）的关键差别：
/// **空结果不退出循环**。超时会在完全没人操作服务器时自然发生，所以扫描器必须常驻；
/// seedance2 那个「没活就 co_return、由路由重新拉起」的写法在这里会漏掉所有超时。
class DOODLELIB_API ai_light_suspend_scanner {
  std::atomic_bool is_running_{false};

 public:
  static ai_light_suspend_scanner& Get();
  /// 幂等：已在跑就直接返回
  void run();
  bool is_running() const { return is_running_; }

 private:
  boost::asio::awaitable<void> async_run();
};

}  // namespace doodle::ai_light
