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
#include <doodle_lib/http_client/seedance2_inference_request.h>
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
// 这里曾经有四个用例验 `build_inference_body()`（body_defaults_are_offline_safe、
// body_uses_flat_envelope_shape_with_images、body_attaches_flat_json_schema_format、
// body_sends_instructions_when_given），它们与 `inference_request_t` 结构体一起删除。
//
// **发射路径的覆盖没有丢**：那四个用例验的是「to_json 成扁平信封 + finalize 定稿」这条
// 两步链，而被测的 `build_inference_body` 只是为了方便单测才存在 —— 它把两步拼在一起，
// 于是「最终发出去的 body 长什么样」只能靠调它来验证（见 seedance2_client.h 的说明）。
// 现在两步各自的真身直接可测：`finalize_inference_body` 定义在
// seedance2_inference_request.h 里，是 inline 的纯函数。下面这组用例替掉原来那四个。
//
// 仍然钉住同一条不变量：**发射路径只做减法**（定 model / 关 store），不替调用方补默认值
// （thinking / max_output_tokens / caching 一个都不许被造出来）。

BOOST_AUTO_TEST_CASE(finalize_overrides_model_and_disables_store) {
  nlohmann::json l_body{{"input_text", "hello"}};
  finalize_inference_body(l_body, std::string{doodle_config::g_ark_inference_model});

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
}

BOOST_AUTO_TEST_CASE(finalize_keeps_caller_supplied_values_verbatim) {
  nlohmann::json l_body{
      {"input_text", "看这张图"},
      {"image_detail", "high"},
      {"images", nlohmann::json::array({"/api/seedance2/pictures/a.png", "/api/seedance2/pictures/b.png"})},
      {"max_output_tokens", 1234},
      {"instructions", "你是灯光师"},
      {"temperature", 0.2},
      {"json_schema", nlohmann::json{{"type", "object"}}},
      {"json_schema_name", "lighting_target"},
      {"json_schema_strict", false},
  };
  finalize_inference_body(l_body, std::string{doodle_config::g_ark_inference_model});

  // 定稿只碰 model 与 store: 其余键**原样在场**, 一个都不改、一个都不删。
  BOOST_CHECK_EQUAL(l_body.at("input_text").get<std::string>(), "看这张图");
  BOOST_CHECK_EQUAL(l_body.at("image_detail").get<std::string>(), "high");
  BOOST_CHECK_EQUAL(l_body.at("max_output_tokens").get<std::int64_t>(), 1234);
  BOOST_CHECK_EQUAL(l_body.at("instructions").get<std::string>(), "你是灯光师");
  BOOST_CHECK_EQUAL(l_body.at("temperature").get<double>(), 0.2);
  BOOST_CHECK_EQUAL(l_body.at("json_schema_name").get<std::string>(), "lighting_target");
  BOOST_CHECK_EQUAL(l_body.at("json_schema_strict").get<bool>(), false);
  BOOST_CHECK_EQUAL(l_body.at("json_schema").at("type").get<std::string>(), "object");
  const auto& l_images = l_body.at("images");
  BOOST_CHECK_EQUAL(l_images.size(), 2u);
  BOOST_CHECK_EQUAL(l_images.at(0).get<std::string>(), "/api/seedance2/pictures/a.png");
  BOOST_CHECK_EQUAL(l_images.at(1).get<std::string>(), "/api/seedance2/pictures/b.png");

  // 方舟形状**不由这里制造**：翻译发生在网关透传之后，不在定稿里。
  BOOST_CHECK_EQUAL(l_body.contains("input"), false);
  BOOST_CHECK_EQUAL(l_body.contains("text"), false);
  BOOST_CHECK_EQUAL(l_body.contains("content"), false);
}

BOOST_AUTO_TEST_CASE(finalize_falls_back_to_default_model_on_blank) {
  // 空串(会话没配模型)也要回落到缺省, 不能带着 model:"" 打过去
  nlohmann::json l_body{{"input_text", "x"}, {"model", ""}};
  finalize_inference_body(l_body, std::string{doodle_config::g_ark_inference_model});
  BOOST_CHECK_EQUAL(l_body.at("model").get<std::string>(), std::string{doodle_config::g_ark_inference_model});

  // 前端传了 model 就**以它为准**(服务端不覆盖非空值) —— 与「原样透传」一致
  nlohmann::json l_named{{"input_text", "x"}, {"model", "custom-model"}};
  finalize_inference_body(l_named, std::string{doodle_config::g_ark_inference_model});
  BOOST_CHECK_EQUAL(l_named.at("model").get<std::string>(), "custom-model");
}

// 归一化信封的往返与类型校验用例（inference_request_round_trips_through_struct /
// inference_request_rejects_wrong_typed_keys）随结构体一起删除。它们钉的是「json -> 结构体
// -> json 不丢键、类型不符要抛」—— 现在中间那个结构体不存在了，**丢键这个失败面本身
// 也消失了**：落库的 data_request_ 与发出去的 body 是同一份 json，不再有白名单可以吃键。

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
// 信封的扁平 `images: []` 不是它的输入。这里手工造一份方舟形状的 body 来验改写与签名。
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
