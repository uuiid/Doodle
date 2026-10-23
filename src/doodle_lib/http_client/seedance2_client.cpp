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

namespace doodle::http {

namespace {
constexpr std::string_view g_generations_path{"/api/v3/contents/generations/tasks"};
constexpr std::string_view g_responses_path{"/api/v3/responses"};
}  // namespace

// ─── 工具方法 ────────────────────────────────────────────────────────────────

doodle::seedance2::task_status seedance2_client::parse_status(const nlohmann::json& in_body) {
  return in_body.contains("status") ? in_body.at("status").get<doodle::seedance2::task_status>()
                                    : doodle::seedance2::task_status::failed;
}

bool seedance2_client::is_retryable_error(const nlohmann::json& in_body) {
  if (!in_body.contains("error")) return false;
  auto l_message = in_body.at("error").value("message", std::string{});
  return l_message.find("timeout while fetching resource") != std::string::npos;
}

// ─── 子类方法 ─────────────────────────────────────────────────────────────────

ai_client_base::request_info_t seedance2_client::collect_request_info(const nlohmann::json& in_request) const {
  request_info_t l_info{};
  if (in_request.contains("model")) l_info.model_ = in_request.at("model").get<std::string>();
  if (in_request.contains("resolution")) l_info.resolution_ = in_request.at("resolution").get<std::string>();
  if (in_request.contains("instructions"))
    l_info.text_prompt_ += in_request.at("instructions").get<std::string>() + "\n";
  if (in_request.contains("input_text")) l_info.text_prompt_ += in_request.at("input_text").get<std::string>() + "\n";
  if (in_request.contains("content") && in_request.at("content").is_array())
    for (const auto& l_value : in_request.at("content"))
      if (l_value.contains("type") && l_value.at("type").get<std::string>() == "text" && l_value.contains("text"))
        l_info.text_prompt_ += l_value.at("text").get<std::string>() + "\n";
  return l_info;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::submit_task(
    const doodle::seedance2::task& in_task
) {
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
  l_result.need_retry_ = is_retryable_error(l_result.data_response_);

  co_return l_result;
}

// ─── 推理 (Responses API) ────────────────────────────────────────────────────

ai_client_base::run_task_result_t seedance2_client::parse_inference_response(
    const nlohmann::json& in_body, int in_status_code
) {
  run_task_result_t l_result{};
  l_result.data_response_ = in_body;
  l_result.need_retry_    = is_retryable_error(in_body);

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
  if (in_body.contains("usage") && in_body.at("usage").is_object()) {
    const auto& l_usage     = in_body.at("usage");
    l_result.input_tokens_  = l_usage.value("input_tokens", std::int64_t{0});
    l_result.output_tokens_ = l_usage.value("output_tokens", std::int64_t{0});
    l_result.total_tokens_  = l_usage.value("total_tokens", std::int64_t{0});
  }
  return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::run_infer_task(
    const doodle::seedance2::task& in_task
) {
  const auto l_uid = in_task.user_id_.is_nil() ? std::string{} : boost::uuids::to_string(in_task.user_id_);

  auto l_body      = in_task.data_request_;
  l_body["store"]  = false;

  run_task_result_t l_result{};
  {
    // 每次尝试都重新取 IP 并**重新签名**
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
    co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);
    const auto l_status_code = static_cast<int>(l_res.result_int());
    l_result                 = parse_inference_response(l_res.body(), l_status_code);
  }

  co_return l_result;
}

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::query_task(
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

  run_task_result_t l_result;
  l_result.data_response_ = l_res.body();
  l_result.status_        = parse_status(l_result.data_response_);
  l_result.need_retry_    = is_retryable_error(l_result.data_response_);

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

boost::asio::awaitable<void> seedance2_client::download_result(run_task_result_t& in_data) {
  for (auto& l_url : in_data.result_files_) {
    auto l_raw_file = co_await download_raw_file(l_url);
    auto l_adj_file =
        l_raw_file.parent_path() / (l_raw_file.stem().string() + "_adj" + l_raw_file.extension().string());
    ffmpeg_video_resize{l_raw_file, l_adj_file, core_set::get_set().get_uuid()}.process();
    FSys::remove(l_raw_file);
    in_data.result_file_paths_.push_back(l_adj_file);
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

}  // namespace doodle::http