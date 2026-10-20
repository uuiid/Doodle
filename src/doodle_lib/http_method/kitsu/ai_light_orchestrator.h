//
// AI 灯光：轮次编排与状态机（docs/ue_ai_lighting_plan.md §10.3 / §13.1）
//
// 这一层刻意**不包含任何 HTTP 依赖**：
//   * 单测可以直接调状态机，不必拉起 session_data / JWT；
//   * 测试二进制不会被 http 头污染。
// 路由层（http_method/kitsu/ai_light.h）只是这一层的薄包装。
//
// 本文件不含任何 LLM 接缝调用：状态机只做校验、推进与落库，推理由前端直接调推理网关
// （/session/{}/inference，见 ai_light.h）。角色降级见 docs/ue_ai_lighting_role_downgrade.md。
//
// 「受控例外」：唯一写推理任务行 + 扣费的入口是**推理网关**（ai_light.cpp 的 inference_route），
// 它经由 run_inference_task 落一行任务与对应扣费 —— 推理确实发生了，抹掉它等于抹掉计费凭证。
//

#pragma once

#include <doodle_core/metadata/ai_light/preset.h>
#include <doodle_core/metadata/ai_light/task_payload.h>
#include <doodle_core/metadata/ai_light/view.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/preview_file.h>

#include <doodle_lib/doodle_lib_fwd.h>

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
// 没有「推进入口」这种东西：状态由各提交接口自己推进，不存在「推进」这个空动作。
// 会话的 `draft → generating` 由**参考图上传**推进（reference_route::post）——
// 「存在参考图」就是「可以开始干活」的判据。
//
// 参考图为空时**拒绝**候选提交（400，且不写 failure_reason_）：候选生成的前提是会话有
// 提示词与参考图。因此建会话接口不推进状态 —— 建会话时参考图还不可能存在
// （见 shot_session_route::post）。
//
// 失败时**不落库**：同步请求的失败原因由 HTTP 响应体描述
// （http_session_data.cpp:313 把 doodle_error::what() 放进 body），前端无论状态码是多少
// 都读 body 显示。异步失败（工作机回传渲染结果）没有人可对话，只记 WARN 日志。

/// 回传定位（D54）：队列路线用任务 uuid 找轮次，引擎内路线没有任务行，改用「会话 + 轮次号」。
///
/// 存在的理由：`mark_iteration_rendered` 与 `locate_iteration`
/// 两段都要先定位轮次，而两条入口（worker 回传、本地客户端回传）只差这一步。
/// 用可空任务号 + 可空会话号表达，而不是重载两个函数：重载会让每个调用点各挑一个，
/// 而「任务号为空时回落到会话号」这条回退逻辑就没人负责了。
struct DOODLELIB_API ai_light_iteration_locator_t {
  /// 会话 uuid。任务号为空时必填。
  uuid session_id_{};
  /// 轮次号（会话内的 1、2、3…，不是轮次 uuid）。任务号为空时必填。
  std::int32_t iteration_{0};
};

/// 候选首帧海报的落点（D44）：**唯一**的计算处。
///
/// 会话 / 轮次 / 候选三个 uuid 都在候选行上（`session_id_` / `iteration_id_` / `uuid_id_`），
/// 所以「候选 → 产物路径」是一次纯计算：候选没有路径列，候选 JSON 里也没有路径键。
/// 需要 ctx 只因为它要 `root_`。
///
/// 为什么把 ctx 做成参数、而不是在函数里取 `g_ctx()`：
///   * 布局基准是 `kitsu_ctx_t::root_`，那是运行期配置，不属于候选行；
///   * 单测（test_main）里 `g_ctx()` 不可用，而 `kitsu_ctx_t` 可以在栈上构造
///     （见 src/test/core/ai_light_paths.cpp 的 make_ctx）。
inline FSys::path candidate_render_path(const http::kitsu_ctx_t& in_ctx, const ai_light_candidate& in_candidate) {
  return in_ctx.get_ai_light_candidate_thumbnail_file(in_candidate.uuid_id_);
}

/// 候选视频的落点（D44/D47）：取自 sd2 的媒体目录，只是文件名是候选 uuid、后缀 `.mp4`。
inline FSys::path candidate_movie_path(const http::kitsu_ctx_t& in_ctx, const ai_light_candidate& in_candidate) {
  return in_ctx.get_ai_light_candidate_movie_file(in_candidate.uuid_id_);
}

/// worker 回传渲染完成后调用（§10.3 的「落 iteration → 置 awaiting_judgement」）。
///
/// 本轮没有 worker（§13.3 未做），所以这个函数只提供入口并纳入单测；
/// worker 里程碑把回传处理接上即可，状态机本身不用改。
/// in_candidates 是回传的候选，只填 render_status_ / metrics_ ——
/// 路径不在回传里，落库时按 candidate_render_path 现算（D44）；frame_count_ 是回传里的
/// **诊断**字段（工作机与 UE 用它判断帧序列是否完整），候选行没有对应的列，不落库。
///
/// in_server_task_info_id 为 nil 时按 in_locator（会话 + 轮次号）定位（D54）。
DOODLELIB_API boost::asio::awaitable<void> mark_iteration_rendered(
    sqlite_database& in_sql, const uuid& in_server_task_info_id, const ai_light_iteration_locator_t& in_locator,
    const std::vector<ai_light_candidate>& in_candidates
);

/// 组装会话详情读模型。会话不存在时返回 nullopt。
DOODLELIB_API std::optional<ai_light_session_detail_t> build_session_detail(
    sqlite_database& in_sql, const uuid& in_session_id
);

}  // namespace doodle::ai_light