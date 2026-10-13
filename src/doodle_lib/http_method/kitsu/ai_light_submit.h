#pragma once

#include "core/http/http_function.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_lib/core/http/http_route.h"
#include "doodle_lib/doodle_lib_fwd.h"
#include "doodle_lib/http_method/http_jwt_fun.h"

namespace doodle::http::ai_light {

// ===========================================================================
// 候选提交（角色降级 · 第二梯队）
// ===========================================================================
//
// `POST /api/doodle/ai/light/session/{session_id}/iteration`
//
// 降级前，这一轮候选是服务端自己演的：`iterate_route` → `advance_session_local`
// → `advance_session_generating_branch` 排轴 + 调 `generate_candidates` 接缝
// （发一次 LLM 请求 + `map_candidate_result` 解析）→ 落 iteration + 6 个 candidate。
// 那整条路已经随「候选生成归前端」一起删除，本路由是它唯一的替代品。
//
// 现在，**排轴与展开都归前端**（那 ~800 行本来就是纯函数），服务端只剩：
//   1. 收下前端算好的轴计划与 6 个候选；
//   2. 过 IR 准入闸门（候选数、主光、相邻差异 —— 见 ai_light.cpp 的 validate_candidates_ir）；
//   3. 落 iteration + candidate + **算基线差分**（`diff_rigs` 留在服务端，§3.7）；
//   4. 回 `launch_plan`（与从前服务端自己生成时**同一形状**）。
//
// 为什么"提交"不只写数据库而还要给 launch_plan：走的是**引擎内路线**（D52/D53），
// 发起方本机拿着 plan 自己起 UE、收产物、上传、回传。plan 的装配与落库逻辑都在
// `write_submitted_iteration()`（ai_light_orchestrator.h）—— 那里是唯一的一份实现。
//
// 为什么**不**在这里重算候选：重算就等于前端白算了，且两份结果若有差异，
// 落库的那份就不是前端拿去渲染的那份 —— 前端会渲染出一批和库里对不上的灯。
//
// ---------------------------------------------------------------------------
// 请求体
// ---------------------------------------------------------------------------
// {
//   "iteration": 3,                          // 必填，正整数；前端从会话详情里取
//   "strategy": "refine",                    // 可选，映射到 ai_light_iteration_strategy
//   "parent_iteration_id": "…",              // 可选 uuid；缺省 = 上一轮
//   "axes": [                                // 可选（缺省 = 空轴计划 = 基线轮）
//     {"axis": "key_light_position", "steps": 1, "label": "主光左移一档"}
//   ],
//   "candidates": [                          // 必填，长度必须 == g_candidate_count
//     {
//       "candidate_index": 0,
//       "variant_axis": "key_light_position",  // 可选；基线轮为空
//       "variant_value": -1,                   // 可选
//       "variant_label": "主光左移一档",        // 可选
//       "globals": { … },                      // 必填，lighting_rig_t::globals_ 的形状
//       "lights": [ … ],                       // 必填，lighting_rig_t::lights_ 的形状
//       "ops": [ … ]                           // 可选，审计影子（决定 D-58 路线 A）
//     }
//   ]
// }
//
// **不接受** `session_id` / `operator_id`：session 在路径上，operator 只从 JWT 取。
// 这与网关同一条纪律 —— 结构上不可伪造，比"校验后拒绝"彻底。
//
// ---------------------------------------------------------------------------
// 响应体
// ---------------------------------------------------------------------------
// 会话详情（`build_session_detail` 的形状）+ `launch_plan`（真出了候选才有）。
//
// ---------------------------------------------------------------------------
// 错误码
// ---------------------------------------------------------------------------
// 400 —— body 不是合法 JSON、iteration 非正整数、candidates 长度 != g_candidate_count、
//        闸门任一条不过（缺主光 / 相邻候选 substance 相同）、rig 形状校验不过。
// 404 —— 会话不存在。
// 403 —— 非本项目成员（load_session_checked 内部的 check_project_access）。
// 409 —— 会话状态不是 generating（此时不该提交候选）。
DOODLE_HTTP_JWT_FUN(iteration_submit_route)
DOODLE_HTTP_FUN_OVERRIDE(post)

uuid session_id_{};

DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::ai_light
