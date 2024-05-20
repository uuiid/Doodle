//
// Created by TD on 2024/2/27.
//

#include "task_info.h"

#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/task_server.h>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>
namespace doodle::http {
void task_info::post_task(boost::system::error_code in_error_code, const http_session_data_ptr &in_data) {
  auto l_req  = in_data->get_msg_body_parser<basic_json_body>()->request_parser_->get();
  auto l_body = l_req.body();

  if (!l_body.contains("data")) {
    BOOST_BEAST_ASSIGN_EC(in_error_code, error_enum::bad_json_string);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }
  server_task_info l_task_handle{core_set::get_set().get_uuid(), l_body["data"]};
  if (l_body.contains("source_computer") && l_body["source_computer"].is_string()) {
    l_task_handle.source_computer_ = l_body["source_computer"];
  }
  if (l_body.contains("submitter") && l_body["submitter"].is_string()) {
    l_task_handle.submitter_ = l_body["submitter"];
  }
  // 任务名称
  if (l_body.contains("name") && l_body["name"].is_string()) {
    l_task_handle.name_ = l_body["name"];
  } else {
    l_task_handle.name_ = fmt::format("task_{}", l_task_handle.id_);
  }

  l_task_handle.submit_time_ = chrono::sys_time_pos::clock::now();
  l_task_handle.log_path_    = fmt::format("task_log/{}", core_set::get_set().get_uuid());

  {
    auto l_conn = g_pool_db().get_connection();
    l_task_handle.install_db(l_conn);
  }
  nlohmann::json l_response_json{};
  l_response_json["id"] = l_task_handle;

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.keep_alive(l_req.keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_response_json.dump();
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));

  g_ctx().get<task_server>().add_task(l_task_handle);
  g_ctx().get<task_server>().run();
}

void task_info::get_task(boost::system::error_code in_error_code, const http_session_data_ptr &in_data) {
  auto l_id = in_data->capture_->get<std::string>("id");
  if (!l_id) {
    in_error_code.assign(ERROR_INVALID_DATA, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }

  server_task_info l_task_handle{boost::lexical_cast<uuid>(*l_id)};
  bool is_db = false;
  {
    auto l_conn = g_pool_db().get_connection();
    is_db       = l_task_handle.select_db(l_conn);
  }
  if (!is_db) {
    in_error_code.assign(ERROR_CONTROL_ID_NOT_FOUND, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }

  nlohmann::json l_json = l_task_handle;
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_data->request_parser_->get().version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_data->request_parser_->get().keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));
}
void task_info::list_task(boost::system::error_code in_error_code, const http_session_data_ptr &in_data) {
  std::vector<server_task_info> l_tasks{};
  {
    auto l_conn = g_pool_db().get_connection();
    l_tasks     = server_task_info::select_all(l_conn);
  }
  auto &l_req = in_data->request_parser_->get();
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(l_req.keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = nlohmann::json{l_tasks}.dump();
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));
}
void task_info::get_task_logger(boost::system::error_code in_error_code, const http_session_data_ptr &in_data) {
  auto l_cap = in_data->capture_;
  auto l_id  = l_cap->get<std::string>("id");
  if (!l_id) {
    in_error_code.assign(ERROR_INVALID_DATA, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }

  server_task_info l_task_handle{boost::lexical_cast<uuid>(*l_id)};

  level::level_enum l_level{level::err};
  auto l_query = in_data->url_.query();
  if (auto l_it = l_query.find("level"); l_it != l_query.npos) {
    l_level = magic_enum::enum_cast<level::level_enum>(l_query.substr(l_it + 6, l_query.find('&', l_it) - l_it - 6))
                  .value_or(level::err);
  }
  auto l_log_path = l_task_handle.get_log_path(l_level);
  if (!FSys::exists(l_log_path)) {
    in_error_code.assign(ERROR_FILE_NOT_FOUND, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }
  boost::beast::http::response<boost::beast::http::file_body> l_response{
      boost::beast::http::status::ok, in_data->request_parser_->get().version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_data->request_parser_->get().keep_alive());
  l_response.set(boost::beast::http::field::content_type, "text/plain");
  l_response.body().open(l_log_path.string().c_str(), boost::beast::file_mode::scan, in_error_code);
  if (in_error_code) {
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));
}
void task_info::delete_task(boost::system::error_code in_error_code, const http_session_data_ptr &in_data) {
  auto l_id = in_data->capture_->get<std::string>("id");
  if (!l_id) {
    in_error_code.assign(ERROR_INVALID_DATA, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }
  server_task_info l_task_handle{boost::lexical_cast<uuid>(*l_id)};
  bool is_db = false;
  {
    auto l_conn = g_pool_db().get_connection();
    l_task_handle.delete_db(l_conn);
  }
  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::ok, in_data->request_parser_->get().version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_data->request_parser_->get().keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));
  g_ctx().get<task_server>().erase_task(l_task_handle.id_);
}
void task_info::reg(doodle::http::http_route &in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task",
          session::make_http_reg_fun(boost::asio::bind_executor(g_io_context(), &task_info::list_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task/{id}",
          session::make_http_reg_fun(boost::asio::bind_executor(g_io_context(), &task_info::get_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task/{id}/log",
          session::make_http_reg_fun(boost::asio::bind_executor(g_io_context(), &task_info::get_task_logger))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "v1/task",
          session::make_http_reg_fun<basic_json_body>(boost::asio::bind_executor(g_io_context(), &task_info::post_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::delete_, "v1/task/{id}",
          session::make_http_reg_fun(boost::asio::bind_executor(g_io_context(), &task_info::delete_task))
      ))

      ;
}

}  // namespace doodle::http
