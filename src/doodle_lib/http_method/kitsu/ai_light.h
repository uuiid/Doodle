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
// 这是**唯一**的建会话入口：建会话 + 跑第一步只有这一条路。归属三键
// （entity / project / 灯光任务）全部从镜头解析 —— URL 里不放 project_id，
// 因为那会给出「镜头与项目不一致」的伪造空间，而且等于第二个入口。
//
// ---------------------------------------------------------------------------
// 为什么会话作用域的动作统一挂 /session/{session_id}/<动作>
// ---------------------------------------------------------------------------
// 会话身份一律走**路径**：动作是对「某个会话」做的。不放请求体的理由：体里带
// session_id 时，服务端要么校验「体和路径是不是同一个」，要么悄悄覆盖其中一个 ——
// 两种做法都留下「两个真相」。**路径为准，体里的 session_id 忽略。**
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

// 参考图取图的站点内路径模板。两处**必须一致**：
//   1) kitsu.cpp 里 reference_image_route 的路由注册（那处只能写字面量 ——
//      operator""_url 是 UDL，接不了变量）；
//   2) 接缝 generate_spec 拼给方舟的 images（站点内相对路径，由 to_public_url
//      补公网前缀 + 短时效签名；绝对磁盘路径 file_path_ 绝不能外发）。
// 放在这里是为了让「模板」只有一个定义，路由注册那边靠注释对齐。
constexpr std::string_view g_reference_image_path_template{"/api/doodle/ai/light/reference/{}/image"};

/// 参考图的站点内取图路径（未签名）。接缝用它，别自己拼字符串。
inline std::string reference_image_url(const uuid& in_id) {
  return fmt::format(g_reference_image_path_template, boost::uuids::to_string(in_id));
}

// GET /api/doodle/ai/light/reference/{id}/image   参考图取图
// 会作为 VLM 判分/推理的输入交给方舟, 所以支持短时效签名 URL(uid 参与签名并真正加载人员,
// handler 里的项目权限校验在签名路径下照常生效)
DOODLE_HTTP_SIGNED_FUN(reference_image_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

// GET /api/doodle/ai/light/candidate/{id}/image   中间候选取图（只有定稿才进 preview_file）
DOODLE_HTTP_SIGNED_FUN(candidate_image_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

// GET  /api/doodle/ai/light/session/{session_id}/spec    Spec 版本列表
// POST /api/doodle/ai/light/session/{session_id}/spec    生成 Spec：draft 出第一版，
//                                                        awaiting_spec_confirm 重出（要求已有参考图，否则 400）
//                                                        已实现（§15.7）：统计聚合 + 一次方舟推理。
//                                                        失败时会话状态回滚，Spec 版本不变（可重试）。
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
//                                                          接缝已落地（§15.8）：排轴 + 一次推理 +
//                                                          落 6 个候选 + 派任务；失败回滚可重试
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

// POST /api/doodle/ai/light/render_report/{task_id}   工作机回传一轮渲染结果（§15.12）
//
// 调用方是 **doodle worker**，不是浏览器：鉴权沿用 data_jobs_instance 那套 JWT
// （工作机的 kitsu_client 用同一组凭据回传任务状态），但不做项目权限检查 ——
// 工作机不是项目成员，它只认任务 uuid。路径参数是 server_task_info 的 uuid，
// 轮次由服务端按它反查（get_ai_light_iteration_by_task），回传体里不带 session/iteration。
DOODLE_HTTP_JWT_FUN(render_report_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid task_id_{};
DOODLE_HTTP_FUN_END()

// 候选产物的上传路径模板。**两处必须一致**：
//   1) kitsu.cpp 里 candidate_file_route 的路由注册（那处只能写字面量 ——
//      operator""_url 是 UDL，接不了变量）；
//   2) 工作机 kitsu_client::upload_ai_light_candidate_file 拼的 URL。工作机二进制
//      不该为了一个字符串去 include 服务端路由头，所以那边照字面量拼，靠这条注释对齐。
//
// 角色枚举本身定义在 doodle_core（`doodle::ai_light::ai_light_candidate_file_role`）：
// 服务端路由与工作机客户端都要拼同一个 URL 段，枚举放两边都能看见的地方才不会漂移。
// 注意写成**全限定名**：本文件就在 `doodle::http::ai_light` 里，直接写 `ai_light::x`
// 会命中内层命名空间自己（命名空间名注入），指向 doodle::http::ai_light::x —— 找不到。
constexpr std::string_view g_candidate_file_path_template{"/api/doodle/ai/light/candidate/{}/file/{}"};

// POST /api/doodle/ai/light/candidate/{id}/file/{role}   工作机上传候选产物（§15.13）
//
//   role = render        → 首帧海报 PNG：网格与列表里显示的那张，必传，缺了这张候选不可判断
//   role = render_movie  → 候选视频 mp4（D45）：美术判断的主要依据，同样必传
//   role = render_linear → 线性 EXR（§7.4）：可选产物，传失败只记一条 WARN，
//                          不影响本轮转 awaiting_judgement（D42）。本轮不产线性图
//
// 落盘位置**不由请求体决定**：服务端按候选行的三个 uuid 现算（candidate_render_path /
// candidate_movie_path，D27/D40），所以工作机既写不到别的候选上，也改不了任何路径 ——
// 它只能「往服务端算好的那个文件名里灌字节」。
// 请求体就是文件本身（application/octet-stream + Content-Disposition，与 up_file_base 同一套约定），
// 成功回 **204**（kitsu_client::upload_asset_file 只认 204）。
// 调用方是 worker（与 render_report_route 同类）：鉴权沿用工作机那套 JWT，
// 不做项目权限检查 —— 工作机不是项目成员。但比回传多一道：候选必须真实存在（否则 404）。
DOODLE_HTTP_JWT_FUN(candidate_file_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
doodle::ai_light::ai_light_candidate_file_role role_{doodle::ai_light::ai_light_candidate_file_role::render};
DOODLE_HTTP_FUN_END()

// GET /api/doodle/ai/light/candidate/{id}/movie   取候选视频（视频流）
//
// 与取海报的 candidate_image_route 是同一张图的两种取法：前端网格用海报（轻），
// 点开某个候选播放视频（重）。响应体是 mp4 文件本体，`session_data::make_msg(path)`
// 会带 Range 支持，浏览器能拖动进度。
DOODLE_HTTP_JWT_FUN(candidate_movie_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
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
