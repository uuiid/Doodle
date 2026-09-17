#include "seedance2_client.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/ffmpeg_video.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/lib_warp/boost_fmt_beast.h>

#include <fmt/format.h>

namespace doodle::http::seedance2 {

// ─── 工具方法 ────────────────────────────────────────────────────────────────

nlohmann::json seedance2_client::add_ip_to_req(const nlohmann::json& in_req, std::string_view in_ip) {
  nlohmann::json l_req = in_req;
  for (auto&& l_value : l_req.at("content")) {
    if (l_value.contains("image_url")) {
      auto& l_url = l_value.at("image_url").at("url");
      if (!l_url.get<std::string>().starts_with("http"))
        l_url = fmt::format("http://{}:38192{}", in_ip, l_url.get<std::string>());
    } else if (l_value.contains("video_url")) {
      auto& l_url = l_value.at("video_url").at("url");
      if (!l_url.get<std::string>().starts_with("http"))
        l_url = fmt::format("http://{}:38192{}", in_ip, l_url.get<std::string>());
    } else if (l_value.contains("audio_url")) {
      auto& l_url = l_value.at("audio_url").at("url");
      if (!l_url.get<std::string>().starts_with("http"))
        l_url = fmt::format("http://{}:38192{}", in_ip, l_url.get<std::string>());
    }
  }
  return l_req;
}

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

boost::asio::awaitable<ai_client_base::run_task_result_t> seedance2_client::run_task(const nlohmann::json& in_task) {
  auto l_ip  = co_await get_ip_str();
  auto l_req = add_ip_to_req(in_task, l_ip);

  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, "/api/v3/contents/generations/tasks", 11
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

  co_return l_result;
}

boost::asio::awaitable<ai_client_base::query_task_result_t> seedance2_client::query_task(
    const std::string& in_task_id
) {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, fmt::format("/api/v3/contents/generations/tasks/{}", in_task_id), 11
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

}  // namespace doodle::http::seedance2