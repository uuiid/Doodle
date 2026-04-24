#include "seedance2_client.h"

#include <doodle_core/exception/exception.h>

#include <doodle_lib/core/http/json_body.h>

namespace doodle::http::seedance2 {

boost::asio::awaitable<std::string> seedance2_client::run_task(const nlohmann::json& in_task) {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::post, "/api/v3/contents/generations/tasks", 11
  };
  req.body() = in_task.dump();
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);

  DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "run_task error: {}", l_res.result());

  auto& l_json = l_res.body();
  DOODLE_CHICK(l_json.contains("id"), "run_task response error: {}", l_json.dump());
  co_return l_json.at("id").get<std::string>();
}

boost::asio::awaitable<nlohmann::json> seedance2_client::query_task(const std::string& in_task_id) {
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, fmt::format("/api/v3/contents/generations/tasks/{}", in_task_id), 11
  };
  req.set(boost::beast::http::field::content_type, "application/json");
  req.set(boost::beast::http::field::authorization, fmt::format("Bearer {}", token_));
  boost::beast::http::response<http::basic_json_body> l_res{};
  co_await http_client_ptr_->read_and_write(req, l_res, boost::asio::use_awaitable);

  DOODLE_CHICK(l_res.result() == boost::beast::http::status::ok, "query_task error: {}", l_res.result());

  co_return l_res.body();
}

}  // namespace doodle::http::seedance2