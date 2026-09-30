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
 * 归一化推理信封(方舟 Responses API 的 data_request_)。
 *
 * 这是**唯一**持有信封键名的地方: `seedance2_client::build_inference_body()` 按字段读,
 * `build_spec_request()` 等生产端按字段填。加字段时必须同时加消费逻辑, 否则该字段就是
 * 一个"没人消费的键"——这正是当初把它从 nlohmann::json 提成结构体要消灭的东西。
 *
 * 字段集 == build_inference_body() 读取的键集, 一一对应:
 *   input_text         用户文本           (空则不发该键)
 *   instructions       系统指令           (与 caching 互斥, 这里从不发 caching)
 *   images             图片数组, 站点内路径或绝对 URL (空则不发——**不发 images 空数组**)
 *   image_detail       low|high|auto
 *   model              模型名, 空则由客户端回落到 doodle_config::g_ark_inference_model
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
 * 注意(**禁止**): 不要用结构体去重序列化已经从库里读出来的 data_request_。落库的 json 可能
 * 含本版本不认识的键, 结构体往返会把它们丢掉。落库路径(`http_method/seedance2/task.cpp` 的
 * run_inference_task)必须原样拷贝 json。
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
 * 信封 -> json。emit 规则与改动前 build_spec_request() 手写的键集完全一致: 值等于默认
 * (空串/空数组/未设 optional/非对象 schema)的键一律不写, 所以落库形状没有变化。
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

}  // namespace doodle::http::seedance2
