#include "seedance2_client.h"

#include "doodle_core/configure/static_value.h"
#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <boost/asio/steady_timer.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fmt/format.h>

namespace doodle::http::seedance2 {

namespace {
constexpr std::string_view g_generations_path{"/api/v3/contents/generations/tasks"};
constexpr std::string_view g_responses_path{"/api/v3/responses"};
}  // namespace

// ─── 工具方法 ────────────────────────────────────────────────────────────────

doodle::seedance2::task_status seedance2_client::parse_status(const nlohmann::json& in_body) {
  return in_body.contains("status") ? in_body.at("status").get<doodle::seedance2::task_status>()
                                    : doodle::seedance2::task_status::failed;
}

bool seedance2_client::is_timeout_error(const nlohmann::json& in_body) {
  if (!in_body.contains("error")) return false;
  auto l_message = in_body.at("error").value("message", std::string{});
  return l_message.find("timeout while fetching resource") != std::string::npos;
}

// ─── 子类方法 ─────────────────────────────────────────────────────────────────

ai_client_base::request_info_t seedance2_client::collect_request_info(const nlohmann::json& in_request) const {
  request_info_t l_info{};
  if (in_request.contains("model")) l_info.model_ = in_request.at("model").get<std::string>();
  if (in_request.contains("resolution")) l_info.resolution_ = in_request.at("resolution").get<std::string>();
  // 推理信封(Responses)没有 resolution, 提示词在 input_text/instructions 里
  if (in_request.contains("instructions"))
    l_info.text_prompt_ += in_request.at("instructions").get<std::string>() + "\n";
  if (in_request.contains("input_text")) l_info.text_prompt_ += in_request.at("input_text").get<std::string>() + "\n";
  // content 中 type 为 text 的项按顺序累加为提示词
  if (in_request.contains("content") && in_request.at("content").is_array())
    for (const auto& l_value : in_request.at("content"))
      if (l_value.contains("type") && l_value.at("type").get<std::string>() == "text" && l_value.contains("text"))
        l_info.text_prompt_ += l_value.at("text").get<std::string>() + "\n";
  return l_info;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::run_task(
    const doodle::seedance2::task& in_task
) {
  // 推理任务走同步的 Responses API: 一次调用就有结果, 不做异步提交
  if (in_task.type_ == doodle::seedance2::task_type::inference) co_return co_await run_inference_task(in_task);

  auto l_ip  = co_await get_ip_str();
  auto l_req = add_ip_to_req(
      in_task.data_request_, l_ip, url_sign_secret_,
      in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_)
  );

  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, std::string{g_generations_path}, 11
  };
  req.body() = l_req.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  http_client_ptr_->set_timeout(300s);

  boost::beast::http::response<http::basic_json_body> l_res{};
  try {
    co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
  } catch (...) {
    throw_exception(
        http_request_error{
            boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
        }
    );
  }

  run_task_result_t l_result;
  l_result.data_response_ = l_res.body();
  if (l_result.data_response_.contains("id") && l_res.result() == boost::beast::http::status::ok &&
      !l_result.data_response_.at("id").get<std::string>().empty())
    l_result.task_id_ = l_result.data_response_.at("id").get<std::string>();

  l_result.status_ =
      l_result.task_id_.empty() ? doodle::seedance2::task_status::failed : doodle::seedance2::task_status::queued;
  l_result.is_timeout_ = is_timeout_error(l_result.data_response_);
  l_result.error_code_ = get_error_code(l_result.data_response_);

  co_return l_result;
}

// ─── 推理 (Responses API) ────────────────────────────────────────────────────

nlohmann::json seedance2_client::build_inference_body(const nlohmann::json& in_request) {
  nlohmann::json l_body{};
  // 传了空串(会话没配模型)时也要回落到缺省值, 否则会带着 model:"" 打过去白跑一趟
  const auto l_model = in_request.value("model", std::string{});
  l_body["model"]    = l_model.empty() ? std::string{doodle_config::g_ark_inference_model} : l_model;

  // instructions 与 caching 互斥(方舟会报 400 InvalidParameter), 这里从不发 caching
  if (auto l_v = in_request.value("instructions", std::string{}); !l_v.empty()) l_body["instructions"] = l_v;

  nlohmann::json l_content = nlohmann::json::array();
  if (auto l_text = in_request.value("input_text", std::string{}); !l_text.empty())
    l_content.push_back(nlohmann::json{{"type", "input_text"}, {"text", l_text}});
  if (in_request.contains("images") && in_request.at("images").is_array()) {
    const auto l_detail = in_request.value("image_detail", std::string{});
    for (const auto& l_image : in_request.at("images")) {
      if (!l_image.is_string()) continue;
      // 注意: Responses 的 input_image.image_url 是**字符串**(生成 API 才是 {"url": ...} 对象)
      nlohmann::json l_item{{"type", "input_image"}, {"image_url", l_image.get<std::string>()}};
      if (!l_detail.empty()) l_item["detail"] = l_detail;
      l_content.push_back(std::move(l_item));
    }
  }
  // input 项是 InputItem: type 固定 "message"(文档原话「取值固定为 message」), 不能只发 role。
  l_body["input"] = nlohmann::json::array(
      {nlohmann::json{{"type", "message"}, {"role", "user"}, {"content", std::move(l_content)}}}
  );

  // 不把提示词与素材留在方舟侧
  l_body["store"] = false;
  // max_output_tokens **缺省不发**: 结构化输出要求全字段 required, 输出长度不可预估, 而截断
  // 之后 result_json_ 只剩半个 JSON —— 那个失败面跟"schema 不合规被拒"完全分不出来。
  // 交给模型/账号侧决定上限; 只有调用方显式指定了才发。
  if (in_request.contains("max_output_tokens")) l_body["max_output_tokens"] = in_request.at("max_output_tokens");
  if (in_request.contains("temperature")) l_body["temperature"] = in_request.at("temperature");
  if (in_request.contains("top_p")) l_body["top_p"] = in_request.at("top_p");

  // 缺省关闭思考: 蒸馏/结构化输出不需要思维链, 还能省输出 token
  const auto l_thinking = in_request.value("thinking", std::string{"disabled"});
  if (!l_thinking.empty() && l_thinking != "auto") l_body["thinking"] = nlohmann::json{{"type", l_thinking}};
  if (auto l_effort = in_request.value("reasoning_effort", std::string{}); !l_effort.empty())
    l_body["reasoning"] = nlohmann::json{{"effort", l_effort}};

  if (in_request.contains("json_schema") && in_request.at("json_schema").is_object()) {
    nlohmann::json l_format{
        {"type", "json_schema"},
        {"name", in_request.value("json_schema_name", std::string{"result"})},
        {"schema", in_request.at("json_schema")},
        {"strict", in_request.value("json_schema_strict", true)},
    };
    l_body["text"] = nlohmann::json{{"format", std::move(l_format)}};
  }
  return l_body;
}

std::string seedance2_client::get_error_code(const nlohmann::json& in_body) {
  if (!in_body.contains("error") || !in_body.at("error").is_object()) return {};
  return in_body.at("error").value("code", std::string{});
}

bool seedance2_client::is_sensitive_content_error(const nlohmann::json& in_body) {
  const auto l_code = get_error_code(in_body);
  return l_code.find("SensitiveContent") != std::string::npos || l_code.find("PolicyViolation") != std::string::npos;
}

bool seedance2_client::should_retry(int in_status_code, std::string_view in_error_code) {
  if (in_status_code == 408 || in_status_code == 429 || in_status_code >= 500) return true;
  // 少数并发/配额类错误会带着别的 4xx 回来, 这类是瞬时的, 值得再试一次
  return in_error_code == "InflightBatchsizeExceeded" || in_error_code == "AccountRateLimitExceeded";
}

std::string seedance2_client::extract_output_text(const nlohmann::json& in_body) {
  if (!in_body.contains("output") || !in_body.at("output").is_array())
    return in_body.value("output_text", std::string{});

  std::string l_text{};
  for (const auto& l_item : in_body.at("output")) {
    if (!l_item.is_object()) continue;
    // 思考打开时 output[0] 是 reasoning 项, 只有 message 项才是答案
    if (l_item.value("type", std::string{}) != "message") continue;
    if (!l_item.contains("content") || !l_item.at("content").is_array()) continue;
    for (const auto& l_part : l_item.at("content")) {
      if (!l_part.is_object()) continue;
      if (l_part.value("type", std::string{}) != "output_text") continue;
      l_text += l_part.value("text", std::string{});
    }
  }
  return l_text;
}

std::string seedance2_client::strip_code_fence(std::string_view in_text) {
  const auto l_trim = [](std::string_view in_v) {
    const auto l_begin = in_v.find_first_not_of(" \t\r\n");
    if (l_begin == std::string_view::npos) return std::string_view{};
    return in_v.substr(l_begin, in_v.find_last_not_of(" \t\r\n") - l_begin + 1);
  };

  auto l_view = l_trim(in_text);
  if (!l_view.starts_with("```")) return std::string{l_view};

  const auto l_first_nl = l_view.find('\n');
  if (l_first_nl == std::string_view::npos) return std::string{l_view};
  l_view = l_view.substr(l_first_nl + 1);
  if (const auto l_close = l_view.rfind("```"); l_close != std::string_view::npos) l_view = l_view.substr(0, l_close);
  return std::string{l_trim(l_view)};
}

ai_client_base::run_task_result_t seedance2_client::parse_inference_response(
    const nlohmann::json& in_body, int in_status_code
) {
  run_task_result_t l_result{};
  l_result.data_response_ = in_body;
  l_result.error_code_    = get_error_code(in_body);
  l_result.is_timeout_    = is_timeout_error(in_body);

  if (in_body.contains("id") && in_body.at("id").is_string()) l_result.task_id_ = in_body.at("id").get<std::string>();

  const bool l_http_ok = in_status_code >= 200 && in_status_code < 300;
  if (l_http_ok && !in_body.contains("error")) {
    const auto l_status =
        in_body.contains("status") && in_body.at("status").is_string() ? in_body.at("status").get<std::string>() : "";
    // 没有 status 字段的兼容实现: 有 output 就当作成功
    l_result.status_ = (l_status == "completed" || (l_status.empty() && in_body.contains("output")))
                           ? doodle::seedance2::task_status::succeeded
                           : doodle::seedance2::task_status::failed;
  } else {
    l_result.status_ = doodle::seedance2::task_status::failed;
  }

  if (l_result.status_ == doodle::seedance2::task_status::succeeded) {
    l_result.result_text_ = extract_output_text(in_body);
    if (!l_result.result_text_.empty()) {
      try {
        l_result.result_json_ = nlohmann::json::parse(strip_code_fence(l_result.result_text_));
      } catch (const nlohmann::json::exception&) {
        l_result.result_json_ = nullptr;  // 没要求结构化输出时这里是正常的
      }
    }
  }

  if (in_body.contains("usage") && in_body.at("usage").is_object()) {
    const auto& l_usage       = in_body.at("usage");
    l_result.input_tokens_    = l_usage.value("input_tokens", std::int64_t{0});
    l_result.output_tokens_   = l_usage.value("output_tokens", std::int64_t{0});
    l_result.total_tokens_    = l_usage.value("total_tokens", std::int64_t{0});
  }
  return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::run_inference_task(
    const doodle::seedance2::task& in_task
) {
  const auto l_uid = in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_);
  const auto l_body = build_inference_body(in_task.data_request_);

  run_task_result_t l_result{};
  for (std::size_t l_attempt = 0; l_attempt < doodle_config::g_ark_inference_max_retry; ++l_attempt) {
    // 每次尝试都重新取 IP 并**重新签名**: 重试时旧签名可能已经过期
    auto l_ip       = co_await get_ip_str();
    auto l_req_json = add_ip_to_req(l_body, l_ip, url_sign_secret_, l_uid);

    boost::beast::http::request<boost::beast::http::string_body> req{
        boost::beast::http::verb::post, std::string{g_responses_path}, 11
    };
    req.body() = l_req_json.dump();
    req.set(boost::beast::http::field::content_type, "application/json");
    req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
    req.set(boost::beast::http::field::accept, "application/json");
    req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
    req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
    http_client_ptr_->set_timeout(doodle_config::g_ark_inference_timeout);

    boost::beast::http::response<http::basic_json_body> l_res{};
    bool l_network_error = false;
    try {
      co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
    } catch (const boost::system::system_error& e) {
      if (e.code() == boost::asio::error::operation_aborted) throw;  // 会话被取消, 不重试
      SPDLOG_LOGGER_ERROR(logger_, "推断请求失败(第 {} 次尝试): {}", l_attempt + 1, e.what());
      l_network_error = true;
    } catch (...) {
      SPDLOG_LOGGER_ERROR(
          logger_, "推断请求失败(第 {} 次尝试): {}", l_attempt + 1,
          boost::current_exception_diagnostic_information()
      );
      l_network_error = true;
    }

    if (!l_network_error) {
      const auto l_status_code = static_cast<int>(l_res.result_int());
      l_result                 = parse_inference_response(l_res.body(), l_status_code);
      if (!should_retry(l_status_code, l_result.error_code_)) co_return l_result;
      SPDLOG_LOGGER_WARN(
          logger_, "推断返回 {} ({}), 第 {} 次尝试", l_res.result(), l_result.error_code_, l_attempt + 1
      );
    }

    if (l_attempt + 1 < doodle_config::g_ark_inference_max_retry) {
      boost::asio::steady_timer l_timer{
          co_await boost::asio::this_coro::executor, std::chrono::seconds{1 << l_attempt}
      };
      co_await l_timer.async_wait(boost::asio::use_awaitable);
    }
  }

  // 全是网络错误: data_response_ 还是 null
  if (l_result.data_response_.is_null()) {
    l_result.status_     = doodle::seedance2::task_status::failed;
    l_result.is_timeout_ = true;
  }
  co_return l_result;
}

boost::asio::awaitable<ai_client_base::query_task_result_t> seedance2_client::query_task(
    const doodle::seedance2::task& in_task
) {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, fmt::format("/api/v3/contents/generations/tasks/{}", in_task.task_id_), 11
  };
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");

  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
  DOODLE_CHICK(
      l_res.result() == boost::beast::http::status::ok, "query_task error: {} {}", l_res.result(), l_res.body().dump()
  );

  query_task_result_t l_result;
  l_result.client_ptr_    = shared_from_this();
  l_result.data_response_ = l_res.body();
  l_result.status_        = parse_status(l_result.data_response_);
  l_result.is_timeout_    = is_timeout_error(l_result.data_response_);

  if (l_result.data_response_.contains("content") && l_result.data_response_.at("content").contains("video_url"))
    l_result.result_files_.push_back(l_result.data_response_.at("content").at("video_url").get<std::string>());
  if (l_result.data_response_.contains("usage") && l_result.data_response_.at("usage").contains("completion_tokens"))
    l_result.completion_tokens_ = l_result.data_response_.at("usage").at("completion_tokens").get<std::int64_t>();

  co_return l_result;
}

boost::asio::awaitable<void> seedance2_client::cancel_task(const std::string& in_task_id) {
  boost::beast::http::request<boost::beast::http::empty_body> req{
      boost::beast::http::verb::delete_, fmt::format("/api/v3/contents/generations/tasks/{}", in_task_id), 11
  };
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  boost::beast::http::response<boost::beast::http::empty_body> l_res{};
  co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
  DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "cancel_task error: {}", l_res.result());
}

boost::asio::awaitable<void> seedance2_client::download_result(query_task_result_t* in_data) {
  for (auto& l_url : in_data->result_files_) {
    auto l_raw_file = co_await download_raw_file(l_url);
    auto l_adj_file =
        l_raw_file.parent_path() / (l_raw_file.stem().string() + "_adj" + l_raw_file.extension().string());
    ffmpeg_video_resize{l_raw_file, l_adj_file, core_set::get_set().get_uuid()}.process();
    FSys::remove(l_raw_file);
    in_data->result_file_paths_.push_back(l_adj_file);
  }
}

std::size_t seedance2_client::default_consumed_tokens(const doodle::seedance2::task_type& in_task_type) const {
  switch (in_task_type) {
    case doodle::seedance2::task_type::video:
      return doodle_config::g_max_task_completion_tokens;
    case doodle::seedance2::task_type::inference:
      return doodle_config::g_ark_inference_default_consumed_tokens;
    default:
      return 5000;
  }
}

}  // namespace doodle::http::seedance2