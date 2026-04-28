#include "seedance2_client.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/http/json_body.h>

#include "core/core_set.h"
#include <fmt/format.h>

namespace doodle::http::seedance2 {

boost::asio::awaitable<std::string> seedance2_client::run_task(const nlohmann::json& in_task) {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, "/api/v3/contents/generations/tasks", 11
  };
  req.body() = in_task.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);

  DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "run_task error: {}", l_res.result());

  auto& l_json = l_res.body();
  DOODLE_CHICK(l_json.contains("id"), "run_task response error: {}", l_json.dump());
  co_return l_json.at("id").get<std::string>();
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

boost::asio::awaitable<nlohmann::json> seedance2_client::query_task(const std::string& in_task_id) {
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

  DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "query_task error: {}", l_res.result());

  co_return l_res.body();
}

boost::asio::awaitable<FSys::path> seedance2_client::download_result(const std::string& in_file_url) {
  // 解析文件名

  static std::regex l_url_regex(R"(https?:\/\/[^\/\s]+)");
  // https://ark.cn-beijing.volces.com/contents/generations/xxx.mp4 -> /contents/generations/xxx.mp4
  auto l_url_path                     = std::regex_replace(in_file_url, l_url_regex, "");
  auto l_ip                           = in_file_url.substr(0, in_file_url.size() - l_url_path.size());
  http_client_ptr_t l_http_client_ptr = std::make_shared<http_client_t>(std::move(l_ip), *core_set::get_set().ctx_ptr);

  boost::beast::http::request<boost::beast::http::empty_body> req{boost::beast::http::verb::get, l_url_path, 11};
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  req.set(boost::beast::http::field::accept, "application/json");
  req.set(boost::beast::http::field::host, http_client_ptr_->server_ip_);
  req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " doodle");
  boost::beast::http::response<boost::beast::http::file_body> l_res{};
  auto l_path = core_set::get_set().get_cache_root("http") / (core_set::get_set().get_uuid_str() + ".mp4");
  boost::system::error_code l_ec{};
  l_res.body().open(l_path.generic_string().c_str(), boost::beast::file_mode::write, l_ec);
  if (l_ec) throw_exception(http_request_error{boost::beast::http::status::internal_server_error, l_ec.message()});
  co_await l_http_client_ptr->read_and_write(req, l_res, boost::asio::use_awaitable);
  DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "download_result error: {}", l_res.result());
  co_return l_path;
}

}  // namespace doodle::http::seedance2