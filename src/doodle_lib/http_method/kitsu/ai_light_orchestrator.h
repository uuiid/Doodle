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
#include <doodle_core/metadata/ai_light/task_payload.h>
#include <doodle_core/metadata/ai_light/view.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/doodle_lib_fwd.h>
// 归一化推理信封(强类型)。这个头只依赖 nlohmann/json + STL, 不带任何 HTTP 依赖,
// 所以不会破坏本文件顶部「这一层刻意不包含任何 HTTP 依赖」的约定。
#include <doodle_lib/http_client/seedance2_inference_request.h>

// 本头是**伞形头**：三条接缝、rig 纯运算、共用部件各自成文件，本头把它们与状态机
// 一起收在同一个命名空间下 —— 对外仍然只需包含 ai_light_orchestrator.h 一个头。
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

/// 候选渲染产物的落点（D44）：**唯一**的计算处。
///
/// 会话 / 轮次 / 候选三个 uuid 都在候选行上（`session_id_` / `iteration_id_` / `uuid_id_`），
/// 所以「候选 → 产物路径」是一次纯计算：候选没有路径列，候选 JSON 里也没有路径键。
/// 需要 ctx 只因为它要 `root_`。
///
/// 为什么把 ctx 做成参数、而不是在函数里取 `g_ctx()`：
///   * 布局基准是 `kitsu_ctx_t::root_`，那是运行期配置，不属于候选行；
///   * 单测（test_main）里 `g_ctx()` 不可用，而 `kitsu_ctx_t` 可以在栈上构造
///     （见 src/test/core/ai_light_paths.cpp 的 make_ctx）。
inline FSys::path candidate_render_path(
    const http::kitsu_ctx_t& in_ctx, const ai_light_candidate& in_candidate, bool in_linear = false
) {
  return in_ctx.get_ai_light_candidate_file(
      in_candidate.session_id_, in_candidate.iteration_id_, in_candidate.uuid_id_, in_linear
  );
}

/// 候选视频的落点（D44/D47）：与 candidate_render_path 同一套推导，只是产物换成 `.mp4`。
inline FSys::path candidate_movie_path(const http::kitsu_ctx_t& in_ctx, const ai_light_candidate& in_candidate) {
  return in_ctx.get_ai_light_candidate_movie_file(
      in_candidate.session_id_, in_candidate.iteration_id_, in_candidate.uuid_id_
  );
}

/// 候选 MRQ 帧序列的目录（D44/D47）：只有工作机与诊断用得上，服务端不判它的存在
/// （帧是工作机的中间产物，合成完就没有意义了）。
inline FSys::path candidate_frames_dir(const http::kitsu_ctx_t& in_ctx, const ai_light_candidate& in_candidate) {
  return in_ctx.get_ai_light_candidate_frames_dir(
      in_candidate.session_id_, in_candidate.iteration_id_, in_candidate.uuid_id_
  );
}

/// worker 回传渲染完成后调用（§10.3 的「落 iteration → 置 awaiting_judgement」）。
///
/// 本轮没有 worker（§13.3 未做），所以这个函数只提供入口并纳入单测；
/// worker 里程碑把回传处理接上即可，状态机本身不用改。
/// in_candidates 是回传的候选，只填 render_ms_ / light_count_ / metrics_ ——
/// 路径不在回传里，落库时按 candidate_render_path 现算（D44）；frame_count_ 是回传里的
/// **诊断**字段（工作机与 UE 用它判断帧序列是否完整），候选行没有对应的列，不落库。
DOODLELIB_API boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const std::vector<ai_light_candidate>& in_candidates
);

/// 解析工作机回传的整轮结果（§15.12 的 wire 契约，见 ai_light/task_payload.h）。
///
/// 严格/宽松的分界（决策 D26）：
///   * `candidates`、每个元素的 `candidate_id` / `status` 是**严格**字段 —— 缺了或值不认识
///     就抛 `bad_gateway`。理由：状态不认识时无法判断「这张图能不能给美术看」，
///     猜一个默认值等于把回传错误变成看不出来的偏移。
///   * `candidate_index` / `render_ms` / `light_count` / `metrics` 是**宽松**字段 ——
///     缺失取默认、形状不符丢弃（+ WARN）。它们是数值与诊断数据，丢一个不该让整轮的
///     图白渲染。
///
/// 纯函数（不落库、不读服务端状态），单测直接喂 JSON 即可。
DOODLELIB_API round_render_report_t parse_round_render_report(const nlohmann::json& in_json);

/// 处理一次工作机回传：解析结果 → 落候选行 → 推进轮次/会话状态。
///
/// 两条出口：
///   * 至少一个候选「海报 + 视频」都在（产物落点按 `candidate_render_path()` /
///     `candidate_movie_path()` 现算，D27：回传里没有路径，也没有可信任的入参）→
///     组装候选后交给 `mark_iteration_rendered()`，轮次转 `awaiting_judgement`、起算 4 小时；
///   * 整轮失败（`error_` 非空，或没有任何 `rendered`，或所有 `rendered` 的产物都不在
///     算出来的落点上 —— D41）→ 转 `mark_iteration_failed()`，
///     **不动状态机状态、不动候选行**，只把原因写进 `session.failure_reason_`（D28）。
///     失败不是终态：美术随时可以再点一次 `/iterate`。
///
/// in_ctx 只用于算产物落点，所以是 const 引用。
/// 抛出的错误（都来自解析或归属校验，属「回传不可用」）：任务号查不到轮次 → 404；
/// 候选不属于该轮次 → 400。
DOODLELIB_API boost::asio::awaitable<void> apply_render_report(
    sqlite_database& in_sql, const http::kitsu_ctx_t& in_ctx, const uuid& in_server_task_info_id,
    const round_render_report_t& in_report
);

/// 整轮失败收尾：只写 `session.failure_reason_`（截断到 512），状态与候选行一概不动（D28）。
/// 任务号查不到轮次抛 404。
DOODLELIB_API boost::asio::awaitable<void> mark_iteration_failed(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const std::string& in_reason
);

/// 组装会话详情读模型。会话不存在时返回 nullopt。
DOODLELIB_API std::optional<ai_light_session_detail_t> build_session_detail(
    sqlite_database& in_sql, const uuid& in_session_id
);

}  // namespace doodle::ai_light