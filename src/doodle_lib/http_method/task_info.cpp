//
// Created by TD on 2024/2/27.
//

#include "task_info.h"

#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
namespace doodle::http {
void task_info::post_task(boost::system::error_code in_error_code, entt::handle in_handle) {
  auto &session      = in_handle.get<http_session_data>();
  auto &l_req        = in_handle.get<session::async_read_body<basic_json_body>>();

  auto l_body        = l_req.request_parser_->get().body();
  auto l_task_handle = entt::handle{*g_reg(), g_reg()->create()};
  if (!l_body.contains("data")) {
    BOOST_BEAST_ASSIGN_EC(in_error_code, error_enum::bad_json_string);
    session.seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }
  l_task_handle.emplace<server_task_info>(l_body["data"]);
  if (l_body.contains("source_computer") && l_body["source_computer"].is_string()) {
    l_task_handle.get<server_task_info>().source_computer_ = l_body["source_computer"];
  }
  if (l_body.contains("submitter") && l_body["submitter"].is_string()) {
    l_task_handle.get<server_task_info>().submitter_ = l_body["submitter"];
  }

  l_task_handle.get<server_task_info>().submit_time_ = chrono::sys_time_pos ::clock ::now();

  nlohmann::json l_response_json{};
  l_response_json["id"] = l_task_handle;

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.request_parser_->get().version()
  };
  l_response.keep_alive(l_req.request_parser_->get().keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_response_json.dump();
  l_response.prepare_payload();
  session.seed(std::move(l_response));
}

void task_info::get_task(boost::system::error_code in_error_code, entt::handle in_handle) {
  auto &session = in_handle.get<http_session_data>();
  auto l_cap    = in_handle.get<http_function::capture_t>();
  auto l_id     = l_cap.get<entt::entity>("id");
  if (!l_id) {
    in_error_code.assign(ERROR_INVALID_DATA, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    session.seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }
  auto l_entt = entt::handle{*g_reg(), *l_id};
  if (!l_entt) {
    in_error_code.assign(ERROR_CONTROL_ID_NOT_FOUND, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    session.seed_error(boost::beast::http::status::bad_request, in_error_code);
  }
  nlohmann::json l_json = l_entt.get<server_task_info>();
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, session.request_parser_->get().version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(session.request_parser_->get().keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  session.seed(std::move(l_response));
}
void task_info::list_task(boost::system::error_code in_error_code, entt::handle in_handle) {}

void task_info::reg(doodle::http::http_route &in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task",
          session::make_http_reg_fun<false>(boost::asio::bind_executor(g_io_context(), &task_info::list_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task/{id}",
          session::make_http_reg_fun<false>(boost::asio::bind_executor(g_io_context(), &task_info::get_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "v1/task",
          session::make_http_reg_fun<basic_json_body>(boost::asio::bind_executor(g_io_context(), &task_info::post_task))
      ));
}

}  // namespace doodle::http
