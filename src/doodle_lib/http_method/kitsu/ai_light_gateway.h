//
// AI 灯光：推理网关路由（角色降级 §3.1）
//
// 见 docs/ue_ai_lighting_cpp_plan.md §3.1 与
//    docs/ue_ai_lighting_role_downgrade.md §3.3。
//
// 角色降级之后，三条接缝的**规划逻辑**（提示词组装 / 轴展开 / 模型输出解释）全部搬到前端，
// 服务端只剩三件事：代持凭据、计费、落库。本路由就是「代持凭据 + 计费」那件事的唯一入口。
//
// ---------------------------------------------------------------------------
// 为什么是**一个**网关而不是三条路由
// ---------------------------------------------------------------------------
// 三条接缝在服务端侧剩下的部分**完全相同**：解析触发人 → 建客户端（取该人员所属
// ai_studio 的密钥）→ 建任务行 → 跑推理 → 判成败。它们的差别只有：
//   * 系统指令与 output schema（降级后由前端给，见 §3.1.2）
//   * `text_prompt_`（计费留痕用的展示句）
// 所以按 seam 分三条路由等于把同一段管道抄三遍 —— 而计费口径复制必然慢慢分叉。
//
// ---------------------------------------------------------------------------
// 权限：operator_id **只从 JWT 取，绝不读请求体**
// ---------------------------------------------------------------------------
// 这是网关最重要的安全性质。计费主体是 operator_id（run_inference_task 按 user_id_ 扣
// person_token），所以只要能指定 operator_id，任何登录用户都能拿别人的密钥刷量。
//
// 做法**不是**「读 body 里的 operator_id 再比对后拒绝」，而是**根本不读那个键**：
// 前者依赖「每个分支都记得比对」，后者是结构性保证 —— 没有任何代码路径能让外部值
// 进到 l_base.operator_id_ 里。请求体里塞 operator_id 会被静默忽略。
//
// 会话归属走 load_session_checked 的同一口径（person_.check_project_access），
// 与 judge_route / iterate_route 一致。
//

#pragma once

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <core/http/http_function.h>

namespace doodle::http::ai_light {

// POST /api/doodle/ai/light/session/{}/inference   一次接缝推理（凭据 + 计费 + 原始结果回传）
//
// 挂在会话下（与 /session/{}/spec、/session/{}/iterate 同形），因为：
//   * 权限校验的对象就是会话行，放路径上则「URL 上操作的对象」与「鉴权查的对象」必然同一个；
//   * 计费归属（session_id / project_id）直接从路径那一个 uuid 取，请求体里不再出现 id 字段，
//     于是没有「路径说 A、体里说 B」这种需要额外判定的状态。
//
// 调用方是**浏览器**（降级后前端持有全部规划逻辑），所以：
//   * 走 DOODLE_HTTP_JWT_FUN（浏览器会话 JWT），与 judge_route 同类；
//   * 必须做项目权限检查 —— 与 render_report_route（工作机，不是项目成员）正好相反。
//
// 请求体（全部键见 .cpp 的 build_gateway_inference_request）：
//   seam         "generate_spec" | "generate_candidates" | "structure_feedback"（必填）
//   input_text   前端组装好的完整用户消息（必填）—— **逐字**进任务行的 text_prompt_
//   instructions 可选；缺省落到内置的那一份（/prompt_assets 提供的就是它）
//   json_schema  可选；缺省落到内置的那一份
//   images       可选；base64 参考图
//   其余 inference_request_t 字段（model / temperature / top_p / max_output_tokens /
//   thinking / reasoning_effort / image_detail / json_schema_name / json_schema_strict）
//   可选透传 —— 它们是「这次推理怎么跑」的参数，不构成规划逻辑。
//
// 响应体：{result, seam, input_tokens, output_tokens, total_tokens}
//   **不带 task_id**：seam_inference_result_t 没有这个字段（ai_light_seam_common.h:130-135），
//   要带得改它 + run_inference_task 的返回，不列入第一梯队（见 C++ 计划 §7 第 1 条）。
//   计费留痕在 seedance2::task 行里，可按 text_prompt_ 反查。
//
// 错误码：
//   400  请求体形状违规（缺 seam/input_text、未知接缝名、非法枚举值）
//   404  会话不存在
//   403  非本项目成员（check_project_access）
//   400  内容违规 / 502 推理失败或输出非合法 JSON 对象（由 chick_inference_ok 判定）
DOODLE_HTTP_JWT_FUN(inference_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::ai_light
