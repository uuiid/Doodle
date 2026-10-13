//
// AI 灯光：推理网关处理函数（角色降级 §3.1）
//
// 见 docs/ue_ai_lighting_cpp_plan.md §3.1。
//
// 本文件只做四件事：权限 → 校验入参 → 调 run_seam_inference → 组响应。
// **不组装提示词、不解释模型输出、不碰轴** —— 那三件事降级后都在前端。
//

#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/ai_light/ai_light.h>

#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/ai_light_gateway.h>
#include <doodle_lib/http_method/kitsu/ai_light_json_validation.h>
#include <doodle_lib/http_method/kitsu/ai_light_orchestrator.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/beast/http/status.hpp>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::http::ai_light {

namespace {

// 本文件在 `doodle::http::ai_light` 里，而三个 `build_*_instructions()` /
// `build_*_json_schema()` 与 `run_seam_inference` / `seam_request_base_t`
// 都在 `doodle::ai_light` 里 —— 同名内层命名空间会**遮蔽**外层，直接写
// `ai_light::run_seam_inference` 会命中 `doodle::http::ai_light` 自己（找不到）。
// 与 ai_light.cpp:50 同一写法，这是既有的既定做法，不是本文件新造的。
using namespace doodle::ai_light;

// 三条接缝在服务端侧的**名字**。这些字符串进 seedance2::task 的措辞与日志，
// 必须与三条接缝自己调 run_seam_inference 时传的那个字面量**逐字一致**
// （ai_light_seam_spec.cpp:444 / _candidates.cpp:545 / _feedback.cpp:318），
// 否则「网关路径」与「接缝路径」在日志和计费台账里会分成两套名字，对不上账。
constexpr std::string_view g_seam_spec{"generate_spec"};
constexpr std::string_view g_seam_candidates{"generate_candidates"};
constexpr std::string_view g_seam_feedback{"structure_feedback"};

// 网关 body 的校验域名字。**刻意与接缝名分开**：前者是「前端传错了」，
// 后者是「模型输出不对」—— 两者在日志里必须能一眼区分（接缝名会出现在
// chick_inference_ok 的错误里，用它当校验域会让人误以为问题出在模型）。
constexpr std::string_view g_gateway_domain{"inference"};

/// 接缝名 → 服务端内部 seam 名的白名单映射。
///
/// 为什么用显式白名单而不是 magic_enum 直接转：接缝名是**带下划线的动词短语**
/// （generate_spec），不是合法标识符风格，硬做成 enum 会让三处调用点的字面量
/// 与枚举名之间多一层需要人工对齐的翻译。白名单只有三项，且这张表就是唯一真源。
///
/// 未知名字**抛 400**（不是 502）：请求体是前端给的，给错了是调用方的问题。
/// json_val::require_enum 抛的是 bad_gateway（那是给「模型输出」用的口径），
/// 所以这里不能复用它 —— 见文件头对 g_gateway_domain 的说明。
std::string resolve_seam_name(const nlohmann::json& in_json) {
  const auto l_name = json_val::require_string(in_json, g_gateway_domain, "seam");
  constexpr std::array<std::string_view, 3> l_allowed{g_seam_spec, g_seam_candidates, g_seam_feedback};
  DOODLE_CHICK_HTTP(
      std::find(l_allowed.begin(), l_allowed.end(), l_name) != l_allowed.end(), bad_request,
      "{}：字段 seam 的值 \"{}\" 不是合法接缝（应为 {} / {} / {}）", g_gateway_domain, l_name, g_seam_spec,
      g_seam_candidates, g_seam_feedback
  );
  return l_name;
}

/// 该接缝的**内置**系统指令。前端没给 instructions 时用它。
///
/// 这三份是编译期常量，`GET /api/doodle/ai/light/prompt_assets` 返回的就是它们 ——
/// 「前端不重写模板」这条约束靠两边同源来保证（C++ 计划 §6 风险 2/6）。
std::string builtin_instructions(std::string_view in_seam) {
  if (in_seam == g_seam_spec) return build_spec_instructions();
  if (in_seam == g_seam_candidates) return build_candidates_instructions();
  return build_feedback_instructions();
}

/// 该接缝的**内置** output schema。前端没给 json_schema 时用它。
nlohmann::json builtin_json_schema(std::string_view in_seam) {
  if (in_seam == g_seam_spec) return build_spec_json_schema();
  if (in_seam == g_seam_candidates) return build_candidates_json_schema();
  return build_feedback_json_schema();
}

/// 从网关 body 组装推理信封。
///
/// `input_text` 必填且**逐字**透传：它是前端组装好的完整用户消息，服务端不该理解它的内容。
/// 同一条字符串既进信封也进任务行的 `text_prompt_`（调用点传的），于是「这次推理问了什么」
/// 在计费台账里可读、可反查。
///
/// instructions / json_schema 缺省时落到内置的那一份。其余字段（model / temperature /
/// top_p / max_output_tokens / thinking / reasoning_effort / image_detail /
/// json_schema_name / json_schema_strict）是「这次推理怎么跑」的参数，不是规划逻辑，
/// 所以可选透传 —— 一个都不传时信封与「接缝自己组装的那一份」形状一致。
http::seedance2::inference_request_t build_gateway_inference_request(
    const nlohmann::json& in_json, std::string_view in_seam
) {
  http::seedance2::inference_request_t l_req{};
  l_req.input_text_ = json_val::require_string(in_json, g_gateway_domain, "input_text");

  if (auto l_v = json_val::opt_string(in_json, "instructions"); l_v.has_value())
    l_req.instructions_ = *l_v;
  else
    l_req.instructions_ = builtin_instructions(in_seam);

  // opt_object 在「非 object」时返回 nullopt —— 于是给一个字符串 schema 会**静默**落回
  // 内置值。这是有意的降级（宁可跑内置契约，也不发一个必然被火山拒掉的 schema），
  // 但它必须可发现：调用点在下面记一条 WARN 的判定条件就是它。
  if (auto l_v = json_val::opt_object(in_json, "json_schema"); l_v.has_value())
    l_req.json_schema_ = *l_v;
  else
    l_req.json_schema_ = builtin_json_schema(in_seam);

  if (auto l_v = json_val::opt_string(in_json, "image_detail"); l_v.has_value()) l_req.image_detail_ = *l_v;
  if (auto l_v = json_val::opt_string(in_json, "model"); l_v.has_value()) l_req.model_ = *l_v;
  if (auto l_v = json_val::opt_number(in_json, "temperature"); l_v.has_value()) l_req.temperature_ = *l_v;
  if (auto l_v = json_val::opt_number(in_json, "top_p"); l_v.has_value()) l_req.top_p_ = *l_v;
  if (auto l_v = json_val::opt_string(in_json, "thinking"); l_v.has_value()) l_req.thinking_ = *l_v;
  if (auto l_v = json_val::opt_string(in_json, "reasoning_effort"); l_v.has_value())
    l_req.reasoning_effort_ = *l_v;
  if (auto l_v = json_val::opt_string(in_json, "json_schema_name"); l_v.has_value())
    l_req.json_schema_name_ = *l_v;
  if (auto l_v = json_val::opt_bool(in_json, "json_schema_strict"); l_v.has_value())
    l_req.json_schema_strict_ = *l_v;
  // 参考图。opt_string_array 返回空容器表示「没给」，语义就是「没有参考图」，与信封默认一致。
  l_req.images_ = json_val::opt_string_array(in_json, "images");

  // max_output_tokens 是**唯一**需要单独处理的数值键：opt_number 返回 double，
  // 而信封里它是 int64。不在这里收窄的话，下面的 static_cast 会把「前端传了小数」
  // 静默截断 —— 一个静默的数值偏移比一次明确的失败糟。
  if (auto l_v = json_val::opt_number(in_json, "max_output_tokens"); l_v.has_value()) {
    DOODLE_CHICK_HTTP(
        *l_v == std::floor(*l_v) && *l_v > 0.0, bad_request,
        "{}：字段 max_output_tokens 必须是正整数（收到 {}）", g_gateway_domain, *l_v
    );
    l_req.max_output_tokens_ = static_cast<std::int64_t>(*l_v);
  }

  return l_req;
}

}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(inference_route, post) {
  // 外包人员不能触发推理 —— 与 judge_route / iterate_route 同一口径。
  // 它花的是工作室的钱，不是「只看一眼」的只读操作。
  person_.check_not_outsourcer();

  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  // ---- 1) 权限：会话归属（路径里的 {session_id} → check_project_access）----
  //
  // 会话 id 走**路径**（/session/{session_id}/inference），与 /session/{session_id}/spec、
  // /iterate、/judge 完全同形 —— 于是「鉴权查的对象」与「URL 上操作的对象」必然同一个，
  // 不会出现「路径说 A、体里说 B，权限查了 A、计费记到 B」。
  // 也不从体里取 project_id：会话行自己就知道它属于哪个项目。
  //
  // 会话不存在时 get_by_uuid → to_single 抛 sqlite_orm_exception{404}，
  // 由 http_session_data 统一翻成 404（与 load_session_checked 同一口径）。
  auto l_session = l_sql.get_by_uuid<ai_light_session>(session_id_);
  person_.check_project_access(l_session.project_id_);

  // ---- 2) 校验入参 ----
  const auto l_seam_name = resolve_seam_name(l_json);
  auto l_request         = build_gateway_inference_request(l_json, l_seam_name);

  // json_schema 给的不是 object 时会静默落回内置值（见 build_gateway_inference_request）。
  // 这里补一条 WARN 让它可发现 —— 前端传错 schema 却「看起来成功了」是最难查的一类问题。
  if (l_json.contains("json_schema") && !l_json.at("json_schema").is_object()) {
    SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(),
      "AI 灯光推理网关：会话 {} 的 json_schema 不是 object（是 {}），已落回 {} 的内置 schema",
      l_session.uuid_id_, l_json.at("json_schema").type_name(), l_seam_name
    );
  }

  // ---- 3) 计费元数据 ----
  //
  // `operator_id_` **只从 JWT 取**。请求体里若有 operator_id 键会被彻底忽略 ——
  // 这是结构性保证：没有任何代码路径能让外部值流进来（见 ai_light_gateway.h 的说明）。
  // session_id 写入 base 是为了任务归属可查；分项用不到它。
  seam_request_base_t l_base{};
  l_base.operator_id_ = person_.person_.uuid_id_;
  l_base.session_id_  = l_session.uuid_id_;
  l_base.project_id_  = l_session.project_id_;

  // ---- 4) 跑推理（凭据 + 计费 + 成败判定全在 run_seam_inference 里，不复制）----
  //
  // text_prompt_ 传前端的 input_text：会话列表与计费台账上显示的就是「这次问了什么」，
  // 而不是本网关自己编的一句话 —— 降级后服务端不知道这次推理的业务含义，也不该假装知道。
  //
  // iteration_id 留 nil：网关不铸造轮次。降级后轮次由前端在 /session/{}/iteration 提交时
  // 建立，所以这次推理「属于哪一轮」在网关这里还不存在（与蒸馏阶段同形）。
  auto l_result = co_await run_seam_inference(
      l_sql, l_base, l_seam_name, l_request.input_text_, l_request
  );

  SPDLOG_LOGGER_INFO(
    g_logger_ctrl().get_http(), "AI 灯光推理网关：会话 {} 完成一次 {}（{} → {} token）", l_session.uuid_id_, l_seam_name,
    l_result.input_tokens_, l_result.output_tokens_
  );

  // ---- 5) 组响应。原始结果回传，服务端不做任何解释 ----
  co_return in_handle->make_msg(
      nlohmann::json{
          {"result", l_result.json_},
          {"seam", l_seam_name},
          {"input_tokens", l_result.input_tokens_},
          {"output_tokens", l_result.output_tokens_},
          {"total_tokens", l_result.total_tokens_},
      }
  );
}

}  // namespace doodle::http::ai_light
