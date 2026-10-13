//
// Created by TD on 2024/2/29.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <vector>

namespace doodle::http::seedance2 {

/**
 * 接缝侧自组信封时用的**具名键集**(方舟 Responses API 的 data_request_)。
 *
 * 用途只有一个: 两条接缝(`build_candidates_request()` 等)按字段**填**一份 json 落进
 * `sd2::task.data_request_`。字段名集中在这里, 免得两个生产端各写各的字面量。
 *
 * **它不再参与发射**: `run_inference_task` 把 `data_request_` 原样定稿后发给方舟, 不再经
 * 本结构体往返一趟 —— 那次往返是一份手写白名单, 不认识的前端字段会被静默吃掉, 与
 * 「前端请求原样透传」直接冲突。所以 `to_json` 是**落库形状**的唯一生产者, 出网形状由
 * `finalize_inference_body()` 在原 json 上原地定稿。
 *
 * 键名与语义(方舟 Responses API):
 *   input_text         用户文本           (空则不发该键)
 *   instructions       系统指令           (与 caching 互斥, 这里从不发 caching)
 *   images             图片数组, 站点内路径或绝对 URL (空则不发——**不发 images 空数组**)
 *   image_detail       low|high|auto
 *   model              模型名, 空则由 finalize_inference_body 回落到传入的缺省模型
 *   temperature, top_p, max_output_tokens
 *   thinking           enabled|disabled|auto(缺省 disabled)
 *   reasoning_effort   minimal|low|medium|high
 *   json_schema        结构化输出的 schema(仅 is_object() 时发)
 *   json_schema_name   结构化输出的名字(空则消费者用 "result")
 *   json_schema_strict 是否 strict(空则消费者用 true)
 *
 * 本头刻意只依赖 nlohmann/json 与 STL: `ai_light_orchestrator.h` 要 include 它, 而那一层
 * 明文要求"不包含任何 HTTP 依赖"(单测不必拉起 session_data/JWT)。
 *
 * 注意(**禁止**): 不要用它去反序列化落库的 data_request_。网关透传进来的是整份前端
 * body, 本结构体认识的键只是它的一小部分, 往返一趟就会把其余的键丢掉。
 */
struct DOODLELIB_API inference_request_t {
  std::string input_text_{};
  std::optional<std::string> instructions_{};
  std::vector<std::string> images_{};
  std::optional<std::string> image_detail_{};
  std::optional<std::string> model_{};
  std::optional<double> temperature_{};
  std::optional<double> top_p_{};
  std::optional<std::int64_t> max_output_tokens_{};
  std::optional<std::string> thinking_{};
  std::optional<std::string> reasoning_effort_{};
  nlohmann::json json_schema_{};
  std::string json_schema_name_{};
  std::optional<bool> json_schema_strict_{};

  friend void to_json(nlohmann::json& in_json, const inference_request_t& in_request);
  friend void from_json(const nlohmann::json& in_json, inference_request_t& in_request);
};

/**
 * 信封 -> json(**落库形状**)。emit 规则与改动前接缝手写的键集完全一致:
 * 值等于默认(空串/空数组/未设 optional/非对象 schema)的键一律不写, 所以落库形状没有变化。
 *
 * 注意 `if (in_request.max_output_tokens_)` 走的是 std::optional 的 has_value(), 因此
 * 0.0 / false / 0 都会照常发出——不要改写成 `if (值)`。
 */
inline void to_json(nlohmann::json& in_json, const inference_request_t& in_request) {
  in_json = nlohmann::json::object();
  if (!in_request.input_text_.empty()) in_json["input_text"] = in_request.input_text_;
  if (in_request.instructions_) in_json["instructions"] = *in_request.instructions_;
  if (!in_request.images_.empty()) in_json["images"] = in_request.images_;
  if (in_request.image_detail_) in_json["image_detail"] = *in_request.image_detail_;
  if (in_request.model_) in_json["model"] = *in_request.model_;
  if (in_request.temperature_) in_json["temperature"] = *in_request.temperature_;
  if (in_request.top_p_) in_json["top_p"] = *in_request.top_p_;
  if (in_request.max_output_tokens_) in_json["max_output_tokens"] = *in_request.max_output_tokens_;
  if (in_request.thinking_) in_json["thinking"] = *in_request.thinking_;
  if (in_request.reasoning_effort_) in_json["reasoning_effort"] = *in_request.reasoning_effort_;
  if (in_request.json_schema_.is_object()) in_json["json_schema"] = in_request.json_schema_;
  if (!in_request.json_schema_name_.empty()) in_json["json_schema_name"] = in_request.json_schema_name_;
  if (in_request.json_schema_strict_) in_json["json_schema_strict"] = *in_request.json_schema_strict_;
}

/**
 * json -> 信封。键缺失 => 保持默认值(等价于改动前 .value() 的回落)。
 * 键存在但类型不符 => 抛 nlohmann::json::type_error: 静默丢掉 images 等于静默发出一个没有参考图
 * 的请求, 这种"不一致"比一次明确的失败糟得多。
 *
 * 非对象 json(null/数组)上 contains() 全部为 false, 于是整份信封取默认值——与改动前
 * .value() 在非对象上的行为一致, 不再额外加"必须是对象"的守卫。
 *
 * 只认这组**扁平**键。方舟原生形状的 body(`input` 那套)不再有还原分支: 那是发射路径的事,
 * 而发射路径直接吃 json、根本不经过本结构体(见 finalize_inference_body 的说明)。
 */
inline void from_json(const nlohmann::json& in_json, inference_request_t& in_request) {
  if (in_json.contains("input_text")) in_json.at("input_text").get_to(in_request.input_text_);
  if (in_json.contains("instructions")) in_request.instructions_ = in_json.at("instructions").get<std::string>();
  if (in_json.contains("images")) in_json.at("images").get_to(in_request.images_);
  if (in_json.contains("image_detail")) in_request.image_detail_ = in_json.at("image_detail").get<std::string>();
  if (in_json.contains("model")) in_request.model_ = in_json.at("model").get<std::string>();
  if (in_json.contains("temperature")) in_request.temperature_ = in_json.at("temperature").get<double>();
  if (in_json.contains("top_p")) in_request.top_p_ = in_json.at("top_p").get<double>();
  if (in_json.contains("max_output_tokens"))
    in_request.max_output_tokens_ = in_json.at("max_output_tokens").get<std::int64_t>();
  if (in_json.contains("thinking")) in_request.thinking_ = in_json.at("thinking").get<std::string>();
  if (in_json.contains("reasoning_effort"))
    in_request.reasoning_effort_ = in_json.at("reasoning_effort").get<std::string>();
  if (in_json.contains("json_schema")) in_request.json_schema_ = in_json.at("json_schema");
  if (in_json.contains("json_schema_name"))
    in_request.json_schema_name_ = in_json.at("json_schema_name").get<std::string>();
  if (in_json.contains("json_schema_strict"))
    in_request.json_schema_strict_ = in_json.at("json_schema_strict").get<bool>();
}

/**
 * 定稿一份**要发出去**的 body（原地改）: 覆盖服务端的强制字段。
 *
 * 这是 `/responses` 请求体与「本系统自己的请求」之间的边界: 过了这里的内容就只有方舟的
 * schema 在管, 服务端不再往里加任何字段 —— 所以它**只做覆盖**, 不替前端补默认值。
 *
 * 参数是 `nlohmann::json` 而不是 `inference_request_t`: 发出去的东西就是任务行里那份 json
 * 本身, 中间不再经过一次结构体往返 —— 那次往返是一份手写白名单, 不认识的前端字段会被
 * 静默吃掉, 与「原样透传」直接冲突。
 *
 * `in_default_model` 由调用方从请求体**之外**取（服务端策略）, 于是前端传什么 model 都无效;
 * 空 model（会话没配, 或前端根本没传）也走这条回落, 不会带着 model:"" 打过去。
 */
inline void finalize_inference_body(nlohmann::json& io_body, const std::string& in_default_model) {
  // 传了空串（会话没配模型）时也要回落到缺省值, 否则会带着 model:"" 打过去白跑一趟。
  const auto l_model = io_body.contains("model") && io_body.at("model").is_string()
                           ? io_body.at("model").get<std::string>()
                           : std::string{};
  io_body["model"]   = l_model.empty() ? in_default_model : l_model;
  // 不把提示词与素材留在方舟侧 —— 两条路径同一条隐私口径, 由这里统一保证。
  // store 是**唯一**一个服务端强行写入的键; 除此之外这里不补任何默认值（包括 thinking:
  // 以前这里发 {"type":"disabled"}, 那是服务端替前端做决定, 与「原样透传」冲突）。
  io_body["store"] = false;
  // max_output_tokens 同理**不补**：结构化输出要求全字段 required, 输出长度不可预估, 而截断
  // 之后 result_json_ 只剩半个 JSON —— 那个失败面跟「schema 不合规被拒」完全分不出来。
  // 交给模型/账号侧决定上限; 只有调用方/前端显式给了才发（它在体里, 原样留着, 这里不动）。
}

}  // namespace doodle::http::seedance2
