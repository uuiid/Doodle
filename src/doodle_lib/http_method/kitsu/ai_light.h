//
// AI 灯光：HTTP 路由（docs/ue_ai_lighting_plan.md §13.1 第 5 项）
//
// 只声明路由类；处理函数在 ai_light.cpp，业务编排在 ai_light_orchestrator.{h,cpp}。
// 分成独立头文件而不是塞进 kitsu_reg_url.h（那份已经 850+ 行）：
// 按特性分文件是本工程的既有做法，先例是 seedance2/reg.h。
//
// 全部注册在 create_kitsu_route_2（主服务器）——**不注册到 create_kitsu_local_route**。
// 原因：LLM/VLM 调用只在主服务器上做（只有它持有 kitsu_ctx_t.deepseek_keys_；
// --local 用的是两参构造、没有 key）。客户端只跑 worker 侧（§13.3），不碰这些路由。
//
// ---------------------------------------------------------------------------
// 为什么类名带 _route 后缀
// ---------------------------------------------------------------------------
// 表实体叫 doodle::ai_light::ai_light_session，而路由类位于 doodle::http::ai_light。
// 若路由类也叫 ai_light_session，在 http::ai_light 里这个名字会**遮蔽**实体类型
// （内层命名空间优先），于是 `ai_light_session l_s{};` 拿到的是路由类而不是表实体 ——
// 这是一个编译期能过、语义完全错位的陷阱。加 _route 后缀彻底消除歧义。
//

#pragma once

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <core/http/http_function.h>

namespace doodle::http::ai_light {

// GET  /api/doodle/ai/light/shot/{entity_id}/session   该镜头下的会话列表（含历史，新的在前）
// POST /api/doodle/ai/light/shot/{entity_id}/session   为该镜头建会话**并跑第一步**（唯一入口）
//
// 为什么镜头 id 进**路径**而不是当查询参数：
//   ai_light_session.entity_id_ 是必填（NOT NULL）的镜头 id，会话的归属就是「某个镜头」，
//   所以「这个镜头的会话集合」本身就是一个一等资源。前端镜头页只面对一个 URL。
//
// 为什么不是 /session/{entity_id}（少一层）：
//   /api/doodle/ai/light/session/{id} 已经是「按**会话 id** 取详情 / 删会话」，
//   而镜头 id 与会话 id 都是 uuid，形状上无法区分，路由只能任选其一 ——
//   那是不可接受的歧义（表现为「有时 404 有时 200」，且取决于注册顺序）。
//
// 这是**唯一**的建会话入口：原先并存的
//   POST /api/actions/projects/{project_id}/shots/{id}/ai-light
// 已删除。两者做的是同一件事（建会话 + 跑第一步），
// 而它多带一个 project_id 在路径里，反而给了「镜头与项目不一致」的伪造空间；
// 现在归属三键（entity / project / 灯光任务）全部从镜头解析，没有第二个入口可绕。
//
// ---------------------------------------------------------------------------
// 为什么会话作用域的动作统一挂 /session/{session_id}/<动作>
// ---------------------------------------------------------------------------
// session_id 曾经出现在三个地方：GET /spec 走**查询参数**，各 POST 走**请求体**，
// /reference 走**路径** —— 同一个资源三种写法，前端每接一个接口都要先确认一次。
// 现在统一成路径：动作是对「某个会话」做的，会话身份就该在 URL 上。
//
// 不放请求体的另一个理由：体里带 session_id 时，服务端要么校验「体和路径是不是同一个」，
// 要么悄悄覆盖其中一个 —— 两种做法都留下「两个真相」。**路径为准，体里的 session_id 忽略。**
//
// 不会和 /session/{id}（会话详情）撞，也不会互相前缀吞并：
//   {} 对 uuid 展开成只含十六进制与连字符的正则（http_function.h 的 g_uuid_regex），
//   而路由匹配用的是 std::regex_match —— **整串匹配**。所以 /session/<uuid>/spec 与
//   /session/<uuid>/spec/confirm 互为前缀也不会互相吞掉；路径里给一个畸形 uuid
//   直接不匹配（落到 404），不会进到 from_uuid_str 去抛异常 —— 比查询参数的行为更好。
DOODLE_HTTP_JWT_FUN(shot_session_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid entity_id_{};
DOODLE_HTTP_FUN_END()

// GET    /api/doodle/ai/light/session/{id}   会话详情（六表聚合读模型）
// DELETE /api/doodle/ai/light/session/{id}   删会话（含参考图与候选，目录先进备份）
DOODLE_HTTP_JWT_FUN(session_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// GET  /api/doodle/ai/light/session/{session_id}/reference   参考图列表
// POST /api/doodle/ai/light/session/{session_id}/reference   多图上传（multipart）
DOODLE_HTTP_JWT_FUN(reference_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

// DELETE /api/doodle/ai/light/reference/{id}   删单张参考图（含文件）
DOODLE_HTTP_JWT_FUN(reference_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// GET /api/doodle/ai/light/reference/{id}/image   参考图取图
DOODLE_HTTP_JWT_FUN(reference_image_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

// GET /api/doodle/ai/light/candidate/{id}/image   中间候选取图（只有定稿才进 preview_file）
DOODLE_HTTP_JWT_FUN(candidate_image_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

// GET  /api/doodle/ai/light/session/{session_id}/spec    Spec 版本列表
// POST /api/doodle/ai/light/session/{session_id}/spec    生成 Spec：draft 出第一版，
//                                                        awaiting_spec_confirm 重出（要求已有参考图，否则 400）
//                                                        —— **本轮返回 501**（LLM 接缝未实现）
DOODLE_HTTP_JWT_FUN(spec_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

// POST /api/doodle/ai/light/session/{session_id}/spec/confirm   人工闸门 1：
//                                   确认 Spec（+ 冲突裁决），随后触发首轮候选
DOODLE_HTTP_JWT_FUN(spec_confirm_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

// POST /api/doodle/ai/light/session/{session_id}/iterate   生成一轮候选并派发
//                                                          —— **本轮返回 501**（LLM 接缝未实现）
//                                                          无需请求体：会话在路径上
DOODLE_HTTP_JWT_FUN(iterate_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

// POST /api/doodle/ai/light/session/{session_id}/judge   人工闸门 2：定稿 / 继续迭代 / 全否
DOODLE_HTTP_JWT_FUN(judge_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

// GET  /api/doodle/ai/light/preset         预设库列表（?enabled_only=false 取全部）
// POST /api/doodle/ai/light/preset         新建预设（美术整理的第一批走这里导入）
DOODLE_HTTP_JWT_FUN(preset_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()

// GET/PUT/DELETE /api/doodle/ai/light/preset/{id}
DOODLE_HTTP_JWT_FUN(preset_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::ai_light
