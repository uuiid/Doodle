//
// AI 灯光：HTTP 路由（docs/ue_ai_lighting_plan.md §13.1 第 5 项）
//
// 只声明路由类；处理函数在 ai_light.cpp，业务编排在 ai_light_orchestrator.{h,cpp}。
// 分成独立头文件而不是塞进 kitsu_reg_url.h（那份已经 850+ 行）：
// 按特性分文件是本工程的既有做法，先例是 seedance2/reg.h。
//
// 本文件合并了原 ai_light_gateway.{h,cpp}：推理网关（/session/{}/inference）与上面这批
// 路由**共享同一个命名空间、同一套权限约定、同一个处理文件**，原先拆成两份是为了让
// 「降级后新增的唯一入口」在目录里显眼 —— 代价是每读一次路由表都要跳两个头文件，
// 而 kitsu.cpp 那处注册列表里它们本来就是紧挨着的。
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
#include <doodle_core/metadata/ai_light/task_payload.h>

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
// PUT    /api/doodle/ai/light/session/{id}   改会话（目前只有提示词）
// DELETE /api/doodle/ai/light/session/{id}   删会话（含参考图与候选，目录先进备份）
//
// 为什么「改提示词」在这里而不是 /spec POST 的请求体里：
//   `prompt_text_` 是**会话自己的属性**，不是「生成 Spec」这个动作的参数。原先 /spec POST
//   收 `prompt_text` 顺带写会话行，代价与参考图的排除完全同形（见 reference_instance_route）：
//     * 美术改一句提示词必须调一次「生成 Spec」—— 而改提示词**不该**顺带蒸馏一版方案，
//       原先的写法让「只想改字」的人被迫接受一次 revision+1 与一次方舟推理；
//     * 写 prompt_text_ 的地方于是有两处（这里与 /spec POST），是会分叉的第二条路径。
//   归到本路由后，「谁改会话」只有一个答案。
//
// 那个「改完提示词顺手重出一版」的常见组合动作，改由前端连调两次：先 PUT，再 POST /spec。
// 两次请求换来的是两个语义各自独立 —— 改字失败不会带崩提 Spec，反之亦然。
//
// 请求体（目前只认一个键，给了才改）：
//   {"prompt_text": "…"}   人工提示词原文；**不允许为空串**（空提示词会让蒸馏失去权威意图）
// 响应体：改完的会话详情（与 GET 一致）。
//
// 只收 prompt_text 一项是刻意的：`title_` 也属于「会话自己的属性」，但它目前没有改名入口
// （建会话时缺省用镜头名，见 shot_session_route::post），加进来要连着定「改名算不算一次
// 编辑」这类问题。等真有人要改标题时再往这个 PUT 里加键即可，形状已经留好。
DOODLE_HTTP_JWT_FUN(session_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
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

// GET    /api/doodle/ai/light/reference/{id}   单张参考图详情
// PUT    /api/doodle/ai/light/reference/{id}   改参考图（目前只有排除标记）
// DELETE /api/doodle/ai/light/reference/{id}   删单张参考图（含文件）
//
// 为什么「排除」在这里而不是 /spec POST 的请求体里：
//   `is_outlier_` / `excluded_reason_` 是**参考图自己的属性**，不是「生成 Spec」这个动作的
//   参数。原先 /spec POST 收 `excluded_reference_ids` 顺带改别的资源，代价是：
//     * 改一张图的排除状态必须借道一个语义无关的接口，还要先把它所在会话 id 找出来；
//     * 提 Spec 与排除图两个操作被绑成一次请求，前端没法只做其中一件；
//     * /spec POST 里那段「查图 → 校验归属 → 批量 update」是第二个写 is_outlier_ 的地方，
//       与这里的 PUT 形成两条会分叉的路径（先例：计费口径复制必然慢慢分叉）。
//   归到本路由后，「谁改参考图」只有一个答案。
//
// PUT 请求体（全部字段可选，给了才改）：
//   {"is_outlier": true}                是否排除
//   {"excluded_reason": "构图没主体"}    排除原因；is_outlier=false 时服务端清空它
// 响应体：改完的参考图整行（与 GET 一致）。
DOODLE_HTTP_JWT_FUN(reference_instance_route)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// 参考图取图的站点内路径模板。两处**必须一致**：
//   1) kitsu.cpp 里 reference_image_route 的路由注册（那处只能写字面量 ——
//      operator""_url 是 UDL，接不了变量）；
//   2) 接缝拼给方舟的 images（站点内相对路径，由 to_public_url
//      补公网前缀 + 短时效签名；绝对磁盘路径 file_path_ 绝不能外发）。
// 放在这里是为了让「模板」只有一个定义，路由注册那边靠注释对齐。
constexpr std::string_view g_reference_image_path_template{"/api/doodle/ai/light/reference/{}/image"};

/// 参考图的站点内取图路径（未签名）。接缝用它，别自己拼字符串。
///
/// 曾经的调用方之一（generate_spec）已删除；仍在用的是 candidates 接缝。
/// 蒸馏方案归前端后，前端要自己把参考图 URL 拼进请求体 —— 那时这个模板
/// 就是它该对齐的形状（也是本函数保留的原因）。
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

// 曾经有 GET/POST /api/doodle/ai/light/session/{session_id}/spec（Spec 版本列表 / 生成 Spec）
// 与 POST /api/doodle/ai/light/session/{session_id}/spec/confirm（人工闸门 1）。**两条都已删除。**
//
// `spec` 那条的理由：
//   * POST 是死接口。它最后剩下的动作只有「读会话、校验提示词非空、返回会话详情」，
//     一个字节都不写 —— 而生产代码里**没有任何地方**会 insert 一行 ai_light_spec
//     （只有测试会）。它承载不住自己的名字：既不能生成 Spec，也不是 Spec 的提交口。
//   * 「生成 Spec」这个动作已经不在服务端：它归前端经推理网关自己完成
//     （见 ai_light_orchestrator.h 的接缝说明）。
//   * GET 的 Spec 版本列表是**冗余读**：会话详情（GET /session/{id}，即 build_session_detail
//     的读模型）里已经带了 —— 现在那份数据本身也没了，见下。
//   * 事实核查发现 ai_light_spec 是**一张空表**：全仓没有 insert 路径，于是「已确认的
//     Spec 版本」这个落库概念整条删除（表、ORM 注册、三个读取方法、iteration/rig/载荷里的
//     spec_id 都一并去掉）。后端不再做推理，这份输出版本由前端自己持有。
//
// `spec/confirm` 那条（人工闸门 1 本身）的理由，详见 ai_light.cpp 里那段说明，摘要：
//   闸门 1 的语义是「人工确认服务端蒸馏出的输出版本」，而 Spec 蒸馏早已归前端 ——
//   服务端没有可确认的对象。它的入口状态 `awaiting_spec_confirm` 全仓没有写侧，
//   它写出的 generating 只有 /iteration 提交认，于是它实际只能成功一次，重试必报 400。
//
// 取消后会话的 `draft → generating` 由**参考图上传**推进（reference_route::post）：
// 「有参考图」就是「可以开始干活」的判据，不再需要一次人工确认。

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

// POST /api/doodle/ai/light/render_report_local/{session_id}/{iteration}   引擎内路线回传（D54）
//
// 与上面那条是**同一段逻辑的另一个入口**：轮次不是由任务 uuid 反查的（引擎内路线根本没有
// server_task_info 行，D53），而是直接在路径上给出「会话 + 轮次号」，由
// ai_light_iteration_locator_t 交给 apply_render_report。产物校验（D41/D42）、
// 状态流转、失败收尾全部复用，不复制一份。
//
// 权限与浏览器调用者一致：调用方是本机 Doodle 客户端，用的是**发起人自己的 JWT**
// （不是工作机凭据），所以这里做 check_project_access —— 与 render_report_route
// 「工作机不是项目成员、不查项目权限」的前提正好相反。同一个镜头只有项目成员能渲，
// 这条也没理由放开。
DOODLE_HTTP_JWT_FUN(render_report_local_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
std::int32_t iteration_{0};
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

// POST /api/doodle/ai/light/session/{}/inference   一次接缝推理（凭据 + 计费 + 原始结果回传）
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
// 所以分成三条路由等于把同一段管道抄三遍 —— 而计费口径复制必然慢慢分叉。
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
// 与 judge_route 一致。
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
// 请求体（**原样透传**，见 ai_light.cpp 的 build_gateway_inference_request）：
//   全部键一律逐字打给火山方舟的 /responses，服务端不解释、不重写、不补默认值。
//   形状以方舟 Responses API 为准（input / instructions / tools / text.format / thinking …），
//   于是方舟新增字段、前端用上新能力时，本服务端一行都不用改。
//   服务端在出网前**只**做两件事（finalize_inference_body）：把 model 换成服务端的缺省、
//   强制 store=false（不把提示词与素材留在方舟侧）。除此之外不增删任何键。
//
//   **没有**必需字段校验：什么字段必需、什么值合法全由方舟判定。服务端唯一自己消费的键是
//   `input`，且只用于抽台账摘要 —— 抽不出来也不拒绝请求（只有图片的请求是合法的）。
//
//   注意 `instructions` / `json_schema` **不再有**内置回落：模板归前端设计，服务端不替它决定
//   提示词。旧扁平键（input_text / images / json_schema…）对 /responses 是未知键，被静默忽略
//   —— 给它们报 400 会让「只想改前端」的人被迫先动服务端。
//
// 响应体：{result, input_tokens, output_tokens, total_tokens}
//   **不带 task_id**：seam_inference_result_t 没有这个字段（ai_light_seam_common.h），
//   要带得改它 + run_inference_task 的返回，不列入第一梯队（见 C++ 计划 §7 第 1 条）。
//   计费留痕在 seedance2::task 行里，可按 text_prompt_ 反查。
//
// 错误码：
//   400  body 不是 JSON 对象
//   404  会话不存在
//   403  非本项目成员（check_project_access）
//   400  内容违规 / 502 推理失败或输出非合法 JSON 对象（由 chick_inference_ok 判定）
DOODLE_HTTP_JWT_FUN(inference_route)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid session_id_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::ai_light
