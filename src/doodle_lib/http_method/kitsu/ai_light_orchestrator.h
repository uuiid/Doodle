//
// AI 灯光：轮次编排与状态机（docs/ue_ai_lighting_plan.md §10.3 / §13.1）
//
// 这一层刻意**不包含任何 HTTP 依赖**：
//   * 单测可以直接调状态机，不必拉起 session_data / JWT；
//   * 测试二进制不会被 http 头污染。
// 路由层（http_method/kitsu/ai_light.h）只是这一层的薄包装。
//
// ---------------------------------------------------------------------------
// 三个「接缝」：接入 LLM 的三个位置
// ---------------------------------------------------------------------------
// generate_spec 已落地（见 §15.7）：提示词 + 多图统计 → LightingSpec。
// generate_candidates 已落地（见 §15.8）：Spec + 基线 rig + 上一轮反馈轴 → 本轮 6 个候选。
// structure_feedback 已落地（见 §15.9）：自由文本 → 轴方向，填进
//   ai_light_judgement_t::feedback_axes_，下一轮当**约束**用。
// 三者都走已落地的方舟 Responses 通道（`seedance2_client` + `run_inference_task`，
// 用法见 §15.6）。**仓库里已经没有 not_implemented 的接缝了。**
//
// 接缝契约（实现方必须遵守）：
//   1. 抛出即视为「本轮无法推进」。调用方 advance_session() 会把会话状态回滚到进入时的
//      状态并记录 failure_reason_，所以实现方**不需要**自己清理状态。
//   2. 但必须保证「抛之前没有落库」，或者「落库的部分是幂等可重入的」——
//      调用方只回滚会话状态这一列，不会回滚接缝自己写下的东西。
//      **受控例外**：generate_spec / generate_candidates / structure_feedback 经由
//      run_inference_task 写下一行推理任务与对应的扣费记录。这是有意保留的 —— 推理确实
//      发生了，抹掉它等于抹掉计费凭证。除此之外它们不写任何 ai_light_* 表
//      （返回值全部交给调用方落库）。
//   3. 返回值必须自洽：候选数量 == g_candidate_count，且满足 §6.6 护栏
//      （同轮候选的灯集合与角色完全一致，只有轴参数不同）。调用方会再校验一次，
//      不合规直接判失败 —— 护栏是硬约束，不能靠实现方自觉。
//

#pragma once

#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/view.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/doodle_lib_fwd.h>
// 归一化推理信封(强类型)。这个头只依赖 nlohmann/json + STL, 不带任何 HTTP 依赖,
// 所以不会破坏本文件顶部「这一层刻意不包含任何 HTTP 依赖」的约定。
#include <doodle_lib/http_client/seedance2_inference_request.h>

// 本头是**伞形头**（§15.11）：三条接缝、rig 纯运算、共用部件各自成文件，本头把它们与
// 状态机一起收在同一个命名空间下 —— 对外仍然只需包含 ai_light_orchestrator.h 一个头。
#include "ai_light_rig_ops.h"
#include "ai_light_seam_candidates.h"
#include "ai_light_seam_common.h"
#include "ai_light_seam_feedback.h"
#include "ai_light_seam_spec.h"

#include <boost/asio/awaitable.hpp>
#include <nlohmann/json.hpp>

#include <atomic>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace doodle::ai_light {

// ===========================================================================
// 状态机
// ===========================================================================

/// 会话状态推进的唯一入口。按 in_session.state_ 分派：
///
///   draft / awaiting_spec_confirm（前置：会话已有参考图）
///       → generate_spec 接缝 → 写 ai_light_spec(revision = max+1)
///       → session = awaiting_spec_confirm
///   generating 且已有已确认的 Spec
///       → generate_candidates 接缝 → 写 iteration + g_candidate_count 个 candidate
///       → 建 server_task_info{type = ai_light} → run_next_task() + broadcast
///       → session 保持 generating（渲染中）；渲染完由 mark_iteration_rendered 转 awaiting_judgement
///
/// 参考图为空时**拒绝**（400，且不写 failure_reason_）：Spec 的输入是提示词 + 参考图，
/// 少了参考图就只能蒸出一份 prompt-only 的方案，而那是主输入的静默丢失。
/// 因此建会话接口**不**调本函数 —— 建会话时参考图还不可能存在（见 shot_session_route::post）。
///
/// 失败时回滚会话状态到进入时的状态、记 failure_reason_，然后重新抛出。
/// 这样不会留下卡在 generating 的会话，也不会丢已上传的参考图
/// （参考图的外键 cascade 挂在会话上，删会话才会连带删图）。
///
/// 分派只看状态，所以**同一个状态只能有一个后继步骤** —— 这是结构性约束。
/// 「等判断」的会话没有可推进的步骤（该做的是 /judge，不是再推一步）。
///
/// in_operator_id 是本次推进的触发人（person uuid），转手给 generate_spec 做计费主体与
/// 签名 URL 的 uid。**不给默认值**：见 spec_generation_request_t::operator_id_ 的说明。
/// 为 nil 时回落到 in_session.submitter_；两者都 nil 抛 bad_request（无法解析工作室、
/// 无法计费）—— 这条在 try 内，所以照常回滚并记 failure_reason_。
DOODLELIB_API boost::asio::awaitable<void> advance_session(
    sqlite_database& in_sql, ai_light_session in_session, uuid in_operator_id
);

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

/// 组装会话详情读模型。会话不存在时返回 nullopt。
DOODLELIB_API std::optional<ai_light_session_detail_t> build_session_detail(
    sqlite_database& in_sql, const uuid& in_session_id
);

}  // namespace doodle::ai_light