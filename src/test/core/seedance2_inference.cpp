//
// 推理 (火山方舟 Responses API) 测试
//
// 盯的是 seedance2_client 上的那组纯函数: 请求体构造 / 响应解析 / 重试判定,
// 以及公网素材 URL 的改写(两种请求形状 + 签名)。
// 都不联网、不需要 app_base。
//
// 为什么这些点值得单独钉:
//   1) Responses 的 input_image.image_url 是**字符串**, 生成 API 才是 {"url": ...} 对象 ——
//      两个形状混用会直接 400, 而 400 不会重试, 现场表现为「推理永远失败」;
//   2) thinking 打开时 output[0] 是 reasoning 项, 取 output[0] 会拿到思维链而不是答案;
//   3) store 缺省是 true, 会让提示词与素材留在方舟侧, 与本项目「离线可复现」的约定冲突;
//   4) 站点内素材路由现在要鉴权, 第三方取图必须带短时效签名。
//

#include <doodle_core/configure/static_value.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/http_client/seedance2_client.h>
#include <doodle_lib/http_client/url_sign.h>

#include <boost/test/unit_test.hpp>

#include <map>
#include <string>
#include <string_view>

BOOST_AUTO_TEST_SUITE(seedance2_inference)

namespace {

using namespace doodle;
using namespace doodle::http;
using namespace doodle::http::seedance2;

const std::string g_ip{"1.2.3.4"};
const std::string g_secret{"test-secret"};
const std::string g_uid{"c9f2b8c0-4a1d-4e3b-9f5c-7d6e0a2f3b8d"};

/// 公网前缀由 core_set::public_url_port_ 决定(默认 38192, debug 下 38193), 断言不能写死
std::string g_public_prefix() { return fmt::format("http://{}:{}", g_ip, core_set::get_set().public_url_port_); }

/// 一份「思考 + 答案」的完整响应: output[0] 是 reasoning, output[1] 才是 message
nlohmann::json completed_body_with_reasoning(const std::string& in_text) {
  return nlohmann::json{
      {"id", "resp_abc123"},
      {"status", "completed"},
      {"output",
       nlohmann::json::array(
           {nlohmann::json{{"type", "reasoning"}, {"summary", nlohmann::json::array()}},
            nlohmann::json{
                {"type", "message"},
                {"role", "assistant"},
                {"content", nlohmann::json::array({nlohmann::json{{"type", "output_text"}, {"text", in_text}}})}
            }}
       )},
      {"usage",
       nlohmann::json{
           {"input_tokens", 1200},
           {"input_tokens_details", nlohmann::json{{"cached_tokens", 0}}},
           {"output_tokens", 300},
           {"output_tokens_details", nlohmann::json{{"reasoning_tokens", 0}}},
           {"total_tokens", 1500}
       }}
  };
}

std::map<std::string, std::string> parse_query(std::string_view in_url) {
  std::map<std::string, std::string> l_ret{};
  const auto l_pos = in_url.find('?');
  if (l_pos == std::string_view::npos) return l_ret;
  auto l_query = in_url.substr(l_pos + 1);
  while (!l_query.empty()) {
    const auto l_amp = l_query.find('&');
    const auto l_one = l_query.substr(0, l_amp);
    if (const auto l_eq = l_one.find('='); l_eq != std::string_view::npos)
      l_ret[std::string{l_one.substr(0, l_eq)}] = std::string{l_one.substr(l_eq + 1)};
    if (l_amp == std::string_view::npos) break;
    l_query = l_query.substr(l_amp + 1);
  }
  return l_ret;
}

std::string url_path(std::string_view in_url) {
  const auto l_end_of_query = in_url.find('?');
  const auto l_after_scheme = in_url.find("://");
  if (l_after_scheme == std::string_view::npos) return std::string{in_url.substr(0, l_end_of_query)};
  const auto l_slash = in_url.find('/', l_after_scheme + 3);
  if (l_slash == std::string_view::npos) return {};
  const auto l_len = l_end_of_query == std::string_view::npos ? std::string_view::npos : l_end_of_query - l_slash;
  return std::string{in_url.substr(l_slash, l_len)};
}

}  // namespace

// ---------------------------------------------------------------------------
// 请求体构造
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(body_defaults_are_offline_safe) {
  inference_request_t l_request{};
  l_request.input_text_ = "hello";
  const auto l_body     = seedance2_client::build_inference_body(l_request);

  BOOST_CHECK_EQUAL(l_body.at("model").get<std::string>(), std::string{doodle_config::g_ark_inference_model});
  // store 缺省是 true, 必须显式关掉: 提示词/素材不留在方舟侧
  BOOST_CHECK_EQUAL(l_body.at("store").get<bool>(), false);
  // caching 与 instructions 互斥(方舟会 400), 我们从不发 caching
  BOOST_CHECK_EQUAL(l_body.contains("caching"), false);
  // thinking 缺省**不发**: 发射路径只做减法(定 model / 关 store), 不替调用方补默认值。
  // 以前这里发 {"thinking":{"type":"disabled"}}, 那是服务端替前端做决定 —— 与「原样透传」冲突。
  BOOST_CHECK_EQUAL(l_body.contains("thinking"), false);
  // max_output_tokens 缺省**不发**：结构化输出长度不可预估，截断之后的半个 JSON 与
  // 「schema 不合规被拒」的失败面分不出来，所以上限交给模型/账号侧；只有显式指定才发。
  BOOST_CHECK_EQUAL(l_body.contains("max_output_tokens"), false);
  inference_request_t l_with_limit{};
  l_with_limit.input_text_        = "x";
  l_with_limit.max_output_tokens_ = 1234;
  BOOST_CHECK_EQUAL(
      seedance2_client::build_inference_body(l_with_limit).at("max_output_tokens").get<std::int64_t>(), 1234
  );

  // 空串(会话没配模型)也要回落到缺省, 不能带着 model:"" 打过去
  inference_request_t l_blank_model{};
  l_blank_model.input_text_ = "x";
  l_blank_model.model_      = std::string{};
  BOOST_CHECK_EQUAL(
      seedance2_client::build_inference_body(l_blank_model).at("model").get<std::string>(),
      std::string{doodle_config::g_ark_inference_model}
  );
}

// build_inference_body 现在**只做两件事**：to_json 成扁平信封，再交给 finalize_inference_body
// 做减法定稿（定 model / 关 store）。它**不再**把扁平信封翻译成方舟 Responses 形状
// （`input[].content[]`）—— 那次翻译已随「报文透传」删除（见 seedance2_client.h:46-55）：
// 发射路径直接吃前端发来的 body 原样打出去，结构体往返只会把不认识的键静默吃掉。
//
// 所以这里验的是**真的会跑的那条路径**：扁平键逐个在场且原样保留，
// 而方舟形状的键（input/text/reasoning）**一个都不该被造出来**。
BOOST_AUTO_TEST_CASE(body_uses_flat_envelope_shape_with_images) {
  inference_request_t l_request{};
  l_request.input_text_   = "看这张图";
  l_request.images_       = {"/api/seedance2/pictures/a.png", "/api/seedance2/pictures/b.png"};
  l_request.image_detail_ = "high";
  const auto l_body       = seedance2_client::build_inference_body(l_request);

  // 扁平键原样在场：定稿只定 model / store，其余交给方舟侧自己解释。
  BOOST_CHECK_EQUAL(l_body.at("input_text").get<std::string>(), "看这张图");
  BOOST_CHECK_EQUAL(l_body.at("image_detail").get<std::string>(), "high");
  const auto& l_images = l_body.at("images");
  BOOST_CHECK_EQUAL(l_images.size(), 2u);
  BOOST_CHECK_EQUAL(l_images.at(0).get<std::string>(), "/api/seedance2/pictures/a.png");
  BOOST_CHECK_EQUAL(l_images.at(1).get<std::string>(), "/api/seedance2/pictures/b.png");

  // 方舟形状**不由这里制造**：翻译发生在网关透传之后，不在信封往返里。
  BOOST_CHECK_EQUAL(l_body.contains("input"), false);
  BOOST_CHECK_EQUAL(l_body.contains("text"), false);
  BOOST_CHECK_EQUAL(l_body.contains("content"), false);
}

BOOST_AUTO_TEST_CASE(body_attaches_flat_json_schema_format) {
  const auto l_schema = nlohmann::json{
      {"type", "object"}, {"properties", nlohmann::json{{"a", nlohmann::json{{"type", "string"}}}}}
  };
  inference_request_t l_request{};
  l_request.input_text_         = "x";
  l_request.json_schema_        = l_schema;
  l_request.json_schema_name_   = "lighting_target";
  l_request.json_schema_strict_ = false;
  const auto l_body             = seedance2_client::build_inference_body(l_request);

  // schema 三件套以扁平键逐个留存（旧断言找的 text.format 是发射层形状，这里不造）。
  BOOST_CHECK_EQUAL(l_body.at("json_schema_name").get<std::string>(), "lighting_target");
  BOOST_CHECK_EQUAL(l_body.at("json_schema_strict").get<bool>(), false);
  BOOST_CHECK_EQUAL(l_body.at("json_schema").at("type").get<std::string>(), "object");
  // 文档里的形状是**平铺**的(type/name/schema/strict), Python SDK 那种 chat 嵌套变体是文档冲突项。
  BOOST_CHECK_EQUAL(l_body.contains("text"), false);
}

BOOST_AUTO_TEST_CASE(body_sends_instructions_when_given) {
  inference_request_t l_request{};
  l_request.instructions_ = "你是灯光师";
  l_request.input_text_   = "x";
  const auto l_body       = seedance2_client::build_inference_body(l_request);
  BOOST_CHECK_EQUAL(l_body.at("instructions").get<std::string>(), "你是灯光师");
  BOOST_CHECK_EQUAL(l_body.contains("caching"), false);
}

// 归一化信封现在是强类型(struct inference_request_t): json -> 结构体 -> json 必须原样往返,
// 而「键存在但类型不符」必须抛错而不是静默丢掉 —— 丢掉 images 等于发出一个没有参考图的请求,
// 这类静默不一致正是结构体化要消灭的东西。
BOOST_AUTO_TEST_CASE(inference_request_round_trips_through_struct) {
  const nlohmann::json l_source{
      {"input_text", "x"},
      {"instructions", "你是灯光师"},
      {"images", nlohmann::json::array({"/api/a.png"})},
      {"image_detail", "high"},
      {"model", "m"},
      {"temperature", 0.2},
      {"top_p", 0.9},
      {"max_output_tokens", 1234},
      {"thinking", "enabled"},
      {"reasoning_effort", "low"},
      {"json_schema", nlohmann::json{{"type", "object"}}},
      {"json_schema_name", "lighting_spec"},
      {"json_schema_strict", false},
  };
  const auto l_envelope      = l_source.get<inference_request_t>();
  const nlohmann::json l_back = l_envelope;
  // 键集与取值都不许漂: 落库的 data_request_ 就是这份 json
  BOOST_CHECK_EQUAL(l_back == l_source, true);

  // 默认值的键(空串/未设 optional/非对象 schema)不该被写出来 —— 落库形状里没有多余的键
  const nlohmann::json l_minimal = inference_request_t{};
  BOOST_CHECK_EQUAL(l_minimal.empty(), true);

  // 反过来: 键不存在时结构体取默认值, 不抛
  const auto l_default = nlohmann::json{{"input_text", "x"}}.get<inference_request_t>();
  BOOST_CHECK_EQUAL(l_default.images_.empty(), true);
  BOOST_CHECK_EQUAL(l_default.image_detail_.has_value(), false);
  BOOST_CHECK_EQUAL(l_default.json_schema_name_.empty(), true);
  BOOST_CHECK_EQUAL(l_default.json_schema_strict_.has_value(), false);

  // 非对象信封(null): 与改动前 .value() 在非对象上的行为一致, 全部取默认值
  const auto l_from_null = nlohmann::json{}.get<inference_request_t>();
  BOOST_CHECK_EQUAL(l_from_null.input_text_.empty(), true);
}

BOOST_AUTO_TEST_CASE(inference_request_rejects_wrong_typed_keys) {
  // 注意: 这里必须先落到具名 json 变量再交给 BOOST_CHECK_THROW。宏只按**括号**切分实参,
  // 直接把 `nlohmann::json{{"images", "x"}}` 写进宏里, 花括号中的逗号会被当成实参分隔符。
  // images 必须是字符串数组: 类型不符时抛, 而不是悄悄发一个没有参考图的请求
  const nlohmann::json l_images_not_array{{"images", "not-an-array"}};
  const nlohmann::json l_images_int_array{{"images", nlohmann::json::array({1, 2})}};
  const nlohmann::json l_bad_temperature{{"temperature", "warm"}};
  const nlohmann::json l_bad_input_text{{"input_text", 42}};

  BOOST_CHECK_THROW(l_images_not_array.get<inference_request_t>(), nlohmann::json::type_error);
  BOOST_CHECK_THROW(l_images_int_array.get<inference_request_t>(), nlohmann::json::type_error);
  BOOST_CHECK_THROW(l_bad_temperature.get<inference_request_t>(), nlohmann::json::type_error);
  BOOST_CHECK_THROW(l_bad_input_text.get<inference_request_t>(), nlohmann::json::type_error);
}
BOOST_AUTO_TEST_CASE(parse_reads_the_message_item_not_the_reasoning_item) {
  const auto l_result = seedance2_client::parse_inference_response(completed_body_with_reasoning("答案是 42"), 200);

  BOOST_CHECK_EQUAL(l_result.status_ == doodle::seedance2::task_status::succeeded, true);
  BOOST_CHECK_EQUAL(l_result.result_text_, "答案是 42");
  BOOST_CHECK_EQUAL(l_result.task_id_, "resp_abc123");
  BOOST_CHECK_EQUAL(l_result.input_tokens_, 1200);
  BOOST_CHECK_EQUAL(l_result.output_tokens_, 300);
  BOOST_CHECK_EQUAL(l_result.total_tokens_, 1500);
  BOOST_CHECK_EQUAL(l_result.error_code_, "");
}

BOOST_AUTO_TEST_CASE(parse_concatenates_multiple_output_text_parts) {
  const auto l_body = nlohmann::json{
      {"status", "completed"},
      {"output", nlohmann::json::array({nlohmann::json{
                     {"type", "message"},
                     {"content", nlohmann::json::array(
                                     {nlohmann::json{{"type", "output_text"}, {"text", "前半"}},
                                      nlohmann::json{{"type", "output_text"}, {"text", "后半"}}}
                                 )}
                 }})}
  };
  BOOST_CHECK_EQUAL(seedance2_client::extract_output_text(l_body), "前半后半");
}

BOOST_AUTO_TEST_CASE(parse_extracts_json_payload_from_fenced_text) {
  const auto l_result = seedance2_client::parse_inference_response(
      completed_body_with_reasoning("```json\n{\"schema_version\": 1}\n```"), 200
  );

  BOOST_CHECK_EQUAL(l_result.status_ == doodle::seedance2::task_status::succeeded, true);
  // 落库保留**原文**(含围栏), 便于排查模型到底回了什么
  BOOST_CHECK_EQUAL(l_result.result_text_.find("```") != std::string::npos, true);
  BOOST_CHECK_EQUAL(l_result.result_json_.at("schema_version").get<int>(), 1);
}

BOOST_AUTO_TEST_CASE(parse_keeps_null_json_when_text_is_not_json) {
  const auto l_result = seedance2_client::parse_inference_response(completed_body_with_reasoning("就是一句话"), 200);
  BOOST_CHECK_EQUAL(l_result.status_ == doodle::seedance2::task_status::succeeded, true);
  BOOST_CHECK_EQUAL(l_result.result_text_, "就是一句话");
  BOOST_CHECK_EQUAL(l_result.result_json_.is_null(), true);
}

BOOST_AUTO_TEST_CASE(parse_reports_error_code_on_http_error) {
  const auto l_body = nlohmann::json{
      {"error", nlohmann::json{{"code", "SensitiveContentDetected.PolicyViolation"}, {"message", "内容不合规"}}}
  };
  const auto l_result = seedance2_client::parse_inference_response(l_body, 400);

  BOOST_CHECK_EQUAL(l_result.status_ == doodle::seedance2::task_status::failed, true);
  BOOST_CHECK_EQUAL(l_result.error_code_, "SensitiveContentDetected.PolicyViolation");
  BOOST_CHECK_EQUAL(seedance2_client::is_sensitive_content_error(l_body), true);
  BOOST_CHECK_EQUAL(l_result.result_text_, "");
}

BOOST_AUTO_TEST_CASE(parse_treats_incomplete_and_failed_as_failure) {
  BOOST_CHECK_EQUAL(
      seedance2_client::parse_inference_response(nlohmann::json{{"status", "incomplete"}}, 200).status_ ==
          doodle::seedance2::task_status::failed,
      true
  );
  // 200 里带 error 字段也要判失败
  BOOST_CHECK_EQUAL(
      seedance2_client::parse_inference_response(
          nlohmann::json{{"status", "completed"}, {"error", nlohmann::json{{"code", "InternalError"}}}}, 200
      )
              .status_ == doodle::seedance2::task_status::failed,
      true
  );
}

BOOST_AUTO_TEST_CASE(strip_code_fence_handles_the_common_shapes) {
  BOOST_CHECK_EQUAL(seedance2_client::strip_code_fence("```json\n{\"a\":1}\n```"), "{\"a\":1}");
  BOOST_CHECK_EQUAL(seedance2_client::strip_code_fence("```\n{\"a\":1}\n```"), "{\"a\":1}");
  BOOST_CHECK_EQUAL(seedance2_client::strip_code_fence("\n  ```json\n{\"a\":1}\n```  \n"), "{\"a\":1}");
  // 没有围栏时原样(只去首尾空白)
  BOOST_CHECK_EQUAL(seedance2_client::strip_code_fence("  {\"a\":1}  "), "{\"a\":1}");
  BOOST_CHECK_EQUAL(seedance2_client::strip_code_fence(""), "");
}

// ---------------------------------------------------------------------------
// 重试判定
// ---------------------------------------------------------------------------
// should_retry 与它的真值表用例已删除: 推理请求的自动重试整个取消
// (见 seedance2_client.cpp 的 run_inference_task), 判定「这个错值不值得再试一次」
// 现在归调用方 —— 它看得见状态码与 error_code, 比服务端盲重试更清楚上下文。
// 下面这些错误分类仍然存在且各有用例: is_sensitive_content_error(内容违规)、
// parse_inference_response(状态码 -> status_/error_code_)。取消的只是**重试**,
// 不是错误分类。

// ---------------------------------------------------------------------------
// 公网素材 URL 改写
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(to_public_url_prefixes_site_paths_and_keeps_foreign_urls) {
  // 站点内路径 -> 补公网前缀(没有密钥时不签名)
  BOOST_CHECK_EQUAL(ai_client_base::to_public_url("/api/x.png", g_ip), g_public_prefix() + "/api/x.png");
  // 绝对 URL 原样(带自己的签名参数)
  BOOST_CHECK_EQUAL(
      ai_client_base::to_public_url("https://oss.example.com/a.png?sign=xyz", g_ip),
      "https://oss.example.com/a.png?sign=xyz"
  );
  // data: 内联 base64 / 其它 scheme 不能被拼成废 URL
  BOOST_CHECK_EQUAL(ai_client_base::to_public_url("data:image/png;base64,AAAA", g_ip), "data:image/png;base64,AAAA");
  BOOST_CHECK_EQUAL(ai_client_base::to_public_url("", g_ip), "");
}

BOOST_AUTO_TEST_CASE(to_public_url_appends_a_verifiable_signature) {
  const auto l_url   = ai_client_base::to_public_url("/api/seedance2/pictures/a.png", g_ip, g_secret, g_uid);
  const auto l_query = parse_query(l_url);

  BOOST_CHECK_EQUAL(l_url.starts_with(g_public_prefix() + "/api/seedance2/pictures/a.png?"), true);
  BOOST_CHECK_EQUAL(l_query.at(url_sign_t::g_uid_key.data()), g_uid);
  // 签名的路径必须与 URL 的路径一致(前缀是签名之后才补上的)
  BOOST_CHECK_EQUAL(url_path(l_url), "/api/seedance2/pictures/a.png");
  BOOST_CHECK_EQUAL(
      url_sign_t::verify(
          url_path(l_url), l_query.at(url_sign_t::g_exp_key.data()), l_query.at(url_sign_t::g_uid_key.data()),
          l_query.at(url_sign_t::g_sig_key.data()), g_secret, url_sign_t::now_seconds()
      ),
      true
  );
}

BOOST_AUTO_TEST_CASE(add_ip_to_req_covers_generation_shape) {
  const auto l_req = nlohmann::json{
      {"content",
       nlohmann::json::array(
           {nlohmann::json{{"type", "text"}, {"text", "hi"}},
            nlohmann::json{{"type", "image_url"}, {"image_url", nlohmann::json{{"url", "/api/a.png"}}}},
            nlohmann::json{{"type", "video_url"}, {"video_url", nlohmann::json{{"url", "/api/b.mp4"}}}},
            nlohmann::json{
                {"type", "audio_url"}, {"audio_url", nlohmann::json{{"url", "https://oss.example.com/c.mp3"}}}
            }}
       )}
  };
  const auto l_out = ai_client_base::add_ip_to_req(l_req, g_ip);

  BOOST_CHECK_EQUAL(l_out.at("content").at(0).at("text").get<std::string>(), "hi");
  BOOST_CHECK_EQUAL(
      l_out.at("content").at(1).at("image_url").at("url").get<std::string>(), g_public_prefix() + "/api/a.png"
  );
  BOOST_CHECK_EQUAL(
      l_out.at("content").at(2).at("video_url").at("url").get<std::string>(), g_public_prefix() + "/api/b.mp4"
  );
  // 外链原样
  BOOST_CHECK_EQUAL(
      l_out.at("content").at(3).at("audio_url").at("url").get<std::string>(), "https://oss.example.com/c.mp3"
  );
  // 原对象不被改动
  BOOST_CHECK_EQUAL(l_req.at("content").at(1).at("image_url").at("url").get<std::string>(), "/api/a.png");
}

// add_ip_to_req 的签名改写只认**方舟生成形状**（`content[].image_url.url`），
// 因为那是它实际会收到的 body —— 请求体由前端按方舟 schema 组好、服务端原样透传，
// 信封的扁平 `images: []` 不是它的输入。这里手工造一份方舟形状的 body 来验改写与签名，
// **不再**经 build_inference_body 绕一圈（那条路已经不产方舟形状了）。
BOOST_AUTO_TEST_CASE(add_ip_to_req_signs_ark_generation_shape) {
  const auto l_req = nlohmann::json{
      {"model", "m"},
      {"content",
       nlohmann::json::array(
           {nlohmann::json{{"type", "text"}, {"text", "看这个"}},
            nlohmann::json{{"type", "image_url"}, {"image_url", nlohmann::json{{"url", "/api/a.png"}}}}}
       )}
  };
  const auto l_out = ai_client_base::add_ip_to_req(l_req, g_ip, g_secret, g_uid);
  const auto l_url = l_out.at("content").at(1).at("image_url").at("url").get<std::string>();

  // 生成形状(object image_url)被改写并签名
  BOOST_CHECK_EQUAL(l_url.starts_with(g_public_prefix() + "/api/a.png?"), true);
  const auto l_query = parse_query(l_url);
  BOOST_CHECK_EQUAL(
      url_sign_t::verify(
          "/api/a.png", l_query.at(url_sign_t::g_exp_key.data()), l_query.at(url_sign_t::g_uid_key.data()),
          l_query.at(url_sign_t::g_sig_key.data()), g_secret, url_sign_t::now_seconds()
      ),
      true
  );
  // 非 URL 字段原样不动
  BOOST_CHECK_EQUAL(l_out.at("content").at(0).at("text").get<std::string>(), "看这个");
}

BOOST_AUTO_TEST_SUITE_END()
