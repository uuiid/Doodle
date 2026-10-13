//
// AI 灯光：提示词素材只读出口（角色降级 §3.3）
//
// 见 docs/ue_ai_lighting_cpp_plan.md §3.3。
//
// ---------------------------------------------------------------------------
// 为什么需要这条路由（它不是「方便前端」，而是一条防漂移的闸门）
// ---------------------------------------------------------------------------
// 降级后提示词模板由前端使用，但**模板本身不能由前端重写**：模板里那些数值口径
// （「对比度 = luma p90 - p10」「key_fill_ratio 是绝对比值，其余轴是增量」）如果前端另抄一份，
// 抄错不会报错 —— 只会让模型按错的口径给数，美术看到「轴变了但图没变」。
//
// 所以真源留在 C++ 的编译期常量里，前端从这里**读取后原样使用**。于是：
//   * 前端不需要在 TypeScript 里重新维护一套（也就不会漂移）；
//   * C++ 侧 `build_*_instructions()` / `build_*_json_schema()` 因此**不能删**
//     —— 它们就是这里回给前端的唯一内容；
//   * 模板改动走构建，与「改一次模板要走构建」这个降级前的瓶颈相比没有恶化 ——
//     因为降级解决的瓶颈是**组装逻辑**（谁跟谁拼、拼哪些字段），不是模板文本本身。
//
// ---------------------------------------------------------------------------
// 命名
// ---------------------------------------------------------------------------
// `name` 用的是流水线阶段的名字（generate_candidates / structure_feedback）。
// 它们只在本路由里被校验，与别处没有耦合 —— 网关那条路径已经不认 `seam` 这个词了
// （请求按方舟 Responses 形状原样透传）。
// 不用 `candidates` / `feedback` 这种短名：那样同一个概念会在两处路由上长出两套名字，
// 而前端要把两套名字对应起来 —— 那张映射表就是下一个漂移点。
//
// 曾经还有第三个素材 `generate_spec`（蒸馏 LightingSpec 的 instructions/schema）。
// 它随 spec 接缝一起删除了：蒸馏方案现在完全归前端 —— 包括那份提示词模板本身。
// 于是「模板不能由前端重写」这条防漂移理由**只对 candidates / feedback 成立**：
// 那两条接缝仍在服务端自组信封、仍共用这里的真源；而 spec 是前端自己调的推理，
// 它用什么模板由前端自己定（服务端不再有可以漂移的对照物）。
//
// 额外提供 `common` —— build_common_lighting_section() 那三段（职责边界 / 性格→灯光语言
// 映射表 / 单位口径）。它被 candidates 的 instructions **包含**，单独暴露是为了：
//   * 前端组装时知道「哪一段是公共的」；
//   * 变更公共段时能一眼看出影响面（它是两条接缝的共享前提）。
//

#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <core/http/http_function.h>

namespace doodle::http::ai_light {

// GET /api/doodle/ai/light/prompt_assets          两条接缝（+ common）的清单
//
// 响应：{assets: [{name, has_schema, instructions_chars, schema_chars}]}
//
// 为什么清单里给**字符数**而不是内容：这是「模板有没有变」的最廉价指纹 ——
// 前端（或人）比对一次就能发现模板动过，不需要把三份可能几百 KB 的文本拉下来。
// 要内容再逐条 GET。**不给 hash**：字符数够用，而 hash 会让人以为它是版本号
// （它不是 —— 改一个标点字符数就可能不变）。
//
// 权限：与其余会话路由同口径 check_not_outsourcer。模板不含任何项目数据，
// 本可以更宽，但「同一模块的读接口权限一致」比「少查一次权限」重要 ——
// 一个没有权限的系统提示词出口将来很容易被拿去做别的事。
DOODLE_HTTP_JWT_FUN(prompt_assets_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()

// GET /api/doodle/ai/light/prompt_assets/{name}   取一份素材
//
// 响应：{name, instructions, json_schema, json_schema_name, json_schema_strict}
//
// `json_schema_name` / `json_schema_strict` 一并回传：它们是「组装这次推理的请求体时
// 该带什么」的一部分。前端若只拿 schema 不带这两个，就得自己在 TS 里写死
// （`ai_light_<阶段名>` 与 true），那又是一处会与 C++ 漂移的常量。
//
// `common` 只有 instructions，没有 schema —— 它的 json_schema 为 null。
//
// name 未知 → 400（不是 404）。**这是刻意的**：/prompt_assets/{name} 里 {name} 是
// 一个二值（+common）的白名单，不在白名单内是「参数非法」（调用方拼错了），
// 而不是「这个资源不存在」—— 后者的语义会让人以为将来可能多出一个接缝，
// 于是前端写出一个动态探测的分支。白名单即全部可能。
DOODLE_HTTP_JWT_FUN(prompt_asset_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
// 类型必须是 `FSys::path` 而不是 `std::string`：`_url(...)` 的路径参数只接受
// uuid / number / 枚举 / FSys::path 等有限类型（http_function.h:159-176 的 static_assert
// 会拒掉 std::string），而 FSys::path 这一支用的正则就是「一个路径段」。
// 与 kitsu_reg_url.h:479 的 data_attachment_files_file::file_name_ 同一写法 ——
// 这是工程里既有的「字符串型路径段」表示法，不是本路由新造的。
FSys::path name_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::ai_light
