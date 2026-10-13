//
// AI 灯光：轮次编排与状态机（docs/ue_ai_lighting_plan.md §10.3 / §13.1）
//
// 这一层刻意**不包含任何 HTTP 依赖**：
//   * 单测可以直接调状态机，不必拉起 session_data / JWT；
//   * 测试二进制不会被 http 头污染。
// 路由层（http_method/kitsu/ai_light.h）只是这一层的薄包装。
//
// ---------------------------------------------------------------------------
// 「接缝」：接入 LLM 的位置
// ---------------------------------------------------------------------------
// **一条都没有了**。三条接缝全部归前端：
//   * generate_spec（提示词 + 多图统计 → LightingSpec）**归前端**：前端直接调推理网关
//     （/session/{}/inference，见 ai_light.h）自己想怎么问就怎么问，
//     服务端不再替它写提示词、也不再解释模型输出。
//   * generate_candidates（Spec + 基线 rig + 反馈轴 → 6 个候选）**归前端**：
//     候选由前端算好、经 POST /session/{}/iteration 提交，服务端只做 IR 准入闸门
//     与落库（见 ai_light_submit.{h,cpp}）。
//   * structure_feedback（自由文本 → 轴方向）**归前端**：judgement 的 feedback_axes_
//     直接来自请求体滑块；文本原样留存，服务端不再替它跑一次推理。
//   三条的删除同属一次角色降级 —— 见 docs/ue_ai_lighting_role_downgrade.md。
//   服务端不再自己演这一半模型；前端要问模型就走推理网关，网关只做凭据与本分计费。
//
// **本文件已不包含任何接缝调用**：状态机只做校验、推进与落库。
// 下面这段「接缝契约」为历史记录保留，它描述的是已验证过的接缝行为 ——
// 若将来再把某条推理拉回服务端，实现方必须重新满足它。
//
// 接缝契约（历史，实现方必须遵守）：
//   1. 抛出即视为「本轮无法推进」。调用方只把原因记进会话的 failure_reason_，
//      **不改会话状态、也不回滚** —— 状态机的分支自身不落库，
//      所以抛错时库里本来就一行没动，实现方**不需要**自己清理状态。
//   2. 但必须保证「抛之前没有落库」，或者「落库的部分是幂等可重入的」——
//      调用方不会回滚接缝自己写下的东西（它根本没有回滚动作）。
//      **受控例外**：接缝经由 run_inference_task 写下一行推理任务
//      与对应的扣费记录。这是有意保留的 —— 推理确实发生了，抹掉它等于抹掉计费凭证。
//      除此之外它不写任何 ai_light_* 表（返回值全部交给调用方落库）。
//   这条契约现在由**推理网关**（ai_light.cpp）继承：它是唯一还会
//   经由 run_seam_inference 写推理任务行的入口。
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

// 本头是**伞形头**：rig 纯运算与共用部件各自成文件，本头把它们与状态机一起收在同一个
// 命名空间下 —— 对外仍然只需包含 ai_light_orchestrator.h 一个头。
//
// 曾经还 include 过 ai_light_seam_feedback.h（接缝 3 的离线部件）。**已删除** ——
// 那条接缝的全部函数在生产里没有任何调用方：judge 路由已不再把美术的自由文本
// 送去结构化（文本原样落库），它的输入正文组装又是唯一消费者
// lighting_spec_t（该类型已随 Spec 闸门删除）。整条链连同它的单测一起移除。
#include "ai_light_rig_ops.h"
#include "ai_light_seam_common.h"

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
// 曾经这里有两个推进入口 advance_session（队列路线）与 advance_session_local（引擎内
// 路线）。三条接缝（generate_spec / generate_candidates / structure_feedback）与候选生成
// 在「角色降级」中全部归了前端之后，两个入口的函数体都只剩恒抛 400 的兜底分支：
//   draft / awaiting_spec_confirm
//       → 曾经是 generate_spec 接缝蒸馏方案。后继是 /session/{}/spec 提交与 /spec/confirm。
//   generating 且已有已确认的 Spec
//       → 曾经是 generate_candidates 接缝 → iteration + g_candidate_count 个 candidate
//         → 建 server_task_info{type = ai_light} → run_next_task() + broadcast。
//         后继是 POST /session/{}/iteration（ai_light_submit.h）：候选由前端算好交上来，
//         服务端只做 IR 闸门 + 落库。
// 两者已一并删除。**人工闸门 1（/spec/confirm）也已取消**，`awaiting_spec_confirm`
// 这个状态随之从枚举里删除 —— 它的语义是「人工确认服务端蒸馏出的输出版本」，
// 而服务端早已不蒸馏，没有可确认的对象。会话的 `draft → generating` 改由
// **参考图上传**推进（reference_route::post）：「存在参考图」就是「可以开始干活」的判据。
// 生成会话状态由此由这些提交接口自己推进，不再有「推进」这个空动作。
//
// 参考图为空时**拒绝**候选提交（400，且不写 failure_reason_）：候选生成的前提是会话有
// 提示词与参考图。因此建会话接口不推进状态 —— 建会话时参考图还不可能存在
// （见 shot_session_route::post）。
//
// 失败时**不落库、不记录 failure_reason_**：同步请求的失败原因由 HTTP 响应体描述
// （http_session_data.cpp:313 把 doodle_error::what() 放进 body），前端无论状态码是多少
// 都读 body 显示。异步失败（工作机回传渲染结果）没有人可对话，仍走
// mark_iteration_failed() 落库。

/// 回传定位（D54）：队列路线用任务 uuid 找轮次，引擎内路线没有任务行，改用「会话 + 轮次号」。
///
/// 存在的理由：`mark_iteration_rendered` / `mark_iteration_failed` / `apply_render_report`
/// 三段都要先定位轮次，而两条入口（worker 回传、本地客户端回传）只差这一步。
/// 用可空任务号 + 可空会话号表达，而不是重载两个函数：重载会让每个调用点各挑一个，
/// 而「任务号为空时回落到会话号」这条回退逻辑就没人负责了。
struct DOODLELIB_API ai_light_iteration_locator_t {
  /// 会话 uuid。任务号为空时必填。
  uuid session_id_{};
  /// 轮次号（会话内的 1、2、3…，不是轮次 uuid）。任务号为空时必填。
  std::int32_t iteration_{0};
};

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
///
/// in_server_task_info_id 为 nil 时按 in_locator（会话 + 轮次号）定位（D54）。
DOODLELIB_API boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::vector<ai_light_candidate>& in_candidates
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
/// in_server_task_info_id 为 nil 时按 in_locator 定位（D54，引擎内路线没有任务行）。
/// 抛出的错误（都来自解析或归属校验，属「回传不可用」）：定位不到轮次 → 404；
/// 候选不属于该轮次 → 400。
DOODLELIB_API boost::asio::awaitable<void> apply_render_report(
    sqlite_database& in_sql, const http::kitsu_ctx_t& in_ctx, const uuid& in_server_task_info_id,
    const ai_light_iteration_locator_t& in_locator, const round_render_report_t& in_report
);

/// 整轮失败收尾：只写 `session.failure_reason_`（不截断），状态与候选行一概不动（D28）。
/// 定位不到轮次抛 404（任务号为空时按 in_locator 的会话 + 轮次号定位，D54）。
DOODLELIB_API boost::asio::awaitable<void> mark_iteration_failed(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::string& in_reason
);

/// 组装会话详情读模型。会话不存在时返回 nullopt。
DOODLELIB_API std::optional<ai_light_session_detail_t> build_session_detail(
    sqlite_database& in_sql, const uuid& in_session_id
);

// ===========================================================================
// 候选落库（角色降级 · 第二梯队）
// ===========================================================================

/// 前端提交一轮候选的请求。
///
/// 存在的理由：候选现在是前端算好交上来的（角色降级 · 第二梯队），服务端只做
/// IR 准入闸门与落库。两条提交路线（队列 / 引擎内）的**后半段完全相同**
/// （钉身份、落 iteration/candidate、算基线差分、装 launch plan），
/// 所以那一段抽成一个函数、两条路线共用 —— 各写一份必然漂移，
/// 而漂移的表现是「本地路线算不出视频路径」这类只在真机上才发现的故障。
///
/// 请求里**没有** `operator_id_`：它只从 JWT 取（结构上不可伪造，比"校验后拒绝"彻底）。
struct DOODLELIB_API submit_iteration_request_t {
  uuid session_id_{};
  uuid project_id_{};
  uuid operator_id_{};
  /// 本轮轮次号（1、2、3…）。调用方已校验 == current_iteration_ + 1。
  std::int32_t iteration_{0};
  /// 本轮轴计划（从前端提交的 `axes` 解析；缺省为空轴计划 = 基线轮）。
  iteration_axes_t axes_{};
  /// **权威状态**：恰好 g_candidate_count 个已展开的 rig。身份字段由本函数补齐。
  std::vector<lighting_rig_t> rigs_{};
  /// 装配推导要它定位镜头（会话的 task_id_）。
  uuid task_id_{};
};

/// 把一轮候选落库，并返回渲染计划。**两条路线共用**。
///
/// in_local_only = true 走引擎内路线（D52/D53）：不建任务行、不派发、不广播，
/// 返回 launch plan 由发起方本机渲染。false 走队列路线（下放工作机）。
///
/// 为什么返回 `std::optional<ai_light_launch_plan_t>` 而不是 out 参数（编码规范）：
/// 队列路线**没有**本机渲染计划可给 —— 调用方看到 nullopt 应当按「已派发」处理。
///
/// 抛出的错误：装配推不出来 → internal_server_error；候选数不为 g_candidate_count
/// → internal_server_error（调用方本应先过闸门，走到这里说明闸门被绕过了）。
///
/// out_broadcast_task_id 是**出参**（不是返回值的一部分，因为返回值已经是「本机渲染计划
/// 或没有」这个二选一）：队列路线上它被写成新建的任务行 uuid，供调用方广播
/// `server_task_info_new_broadcast_t`；引擎内路线不建任务行，所以保持 nil。
///
/// 为什么用出参而不把它塞进返回值：**任务行 uuid 不只是队列路线的产物** —— 它是
/// `ai_light_iteration.server_task_info_id_` 的来源（渲染回传按它反查轮次），而引擎内路线
/// 根本不发生广播。做成返回值的一部分，就必须先造一个「带任务号的空渲染计划」这种
/// 只在一条路线上有意义的形状；出参让「哪条路线关心它」在调用点上直接可见。
DOODLELIB_API boost::asio::awaitable<std::optional<ai_light_launch_plan_t>> write_submitted_iteration(
    sqlite_database& in_sql, const submit_iteration_request_t& in_request, bool in_local_only,
    uuid& out_broadcast_task_id
);

}  // namespace doodle::ai_light