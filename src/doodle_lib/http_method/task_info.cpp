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

  if (!l_body.contains("command") || !l_body["command"].is_array() || !l_body.contains("exe") ||
      !l_body["exe"].is_string()) {
    BOOST_BEAST_ASSIGN_EC(in_error_code, error_enum::bad_json_string);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }
  server_task_info l_task_handle{
      core_set::get_set().get_uuid(), l_body["exe"].get<std::string>(),
      l_body["command"].get<std::vector<std::string>>()
  };
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
  l_task_handle.log_path_    = fmt::format("task_log/{}", l_task_handle.id_);

  auto l_e                   = entt::handle{*g_reg(), g_reg()->create()};
  l_e.emplace<server_task_info>(l_task_handle);

  nlohmann::json l_task_handle_json{};
  l_task_handle_json = l_task_handle;

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.keep_alive(l_req.keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_task_handle_json.dump();
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));

  g_ctx().get<task_server>().add_task(l_e);
  g_ctx().get<task_server>().run();
}

void task_info::get_task(boost::system::error_code in_error_code, const http_session_data_ptr &in_data) {
  auto l_id = in_data->capture_->get("id");
  boost::uuids::uuid l_uuid{};
  {
    std::istringstream l_stream{l_id};
    l_stream >> l_uuid;
    if (l_stream.fail()) {
      in_error_code.assign(ERROR_INVALID_DATA, boost::system::system_category());
      BOOST_ASIO_ERROR_LOCATION(in_error_code);
      in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
      return;
    }
  }

  server_task_info l_task_handle{};
  bool is_db = false;
  {
    auto l_view = std::as_const(*g_reg()).view<const server_task_info>();
    for (auto &&[e, l_ptr] : l_view.each()) {
      if (l_ptr.id_ == l_task_handle.id_) {
        l_task_handle = l_ptr;
        is_db         = true;
        break;
      }
    }
  }

  if (!is_db) {
    in_error_code.assign(ERROR_CONTROL_ID_NOT_FOUND, boost::system::system_category());
    BOOST_ASIO_ERROR_LOCATION(in_error_code);
    in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
    return;
  }

  nlohmann::json l_json{};
  l_json = l_task_handle;
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
  auto l_url_query   = in_data->url_.query();
  std::size_t l_page = 0;
  if (auto l_it = l_url_query.find("page"); l_it != l_url_query.npos) {
    l_page = std::stoul(l_url_query.substr(l_it + 5, l_url_query.find('&', l_it) - l_it - 5));
  }
  std::size_t l_page_size = 100;
  if (auto l_it = l_url_query.find("page_size"); l_it != l_url_query.npos) {
    l_page_size = std::stoul(l_url_query.substr(l_it + 10, l_url_query.find('&', l_it) - l_it - 10));
  }

  std::vector<server_task_info> l_tasks{};
  {
    auto l_view             = std::as_const(*g_reg()).view<const server_task_info>();
    std::size_t l_view_size = l_view.size();
    auto l_view_end         = l_view.begin() + std::clamp((l_page + 1) * l_page_size, 0ull, l_view_size);
    for (auto it = l_view.begin() + std::clamp(l_page * l_page_size, 0ull, l_view_size); it != l_view_end; ++it) {
      auto e = *it;
      l_tasks.emplace_back(l_view.get<server_task_info>(e));
    }
   }
  auto &l_req = in_data->request_parser_->get();

  nlohmann::json l_tasks_json{};
  l_tasks_json = l_tasks;

  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(l_req.keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_tasks_json.dump();
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));
}
void task_info::get_task_logger(boost::system::error_code in_error_code, const http_session_data_ptr &in_data) {
  auto l_cap = in_data->capture_;
  auto l_id  = l_cap->get("id");

  boost::uuids::uuid l_uuid{};
  {
    std::istringstream l_stream{l_id};
    l_stream >> l_uuid;
    if (l_stream.fail()) {
      in_error_code.assign(ERROR_INVALID_DATA, boost::system::system_category());
      BOOST_ASIO_ERROR_LOCATION(in_error_code);
      in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
      return;
    }
  }

  server_task_info l_task_handle{l_uuid};

  level::level_enum l_level{level::err};
  auto l_query = in_data->url_.query();
  if (auto l_it = l_query.find("level"); l_it != l_query.npos) {
    l_level = magic_enum::enum_cast<level::level_enum>(l_query.substr(l_it + 6, l_query.find('&', l_it) - l_it - 6))
                  .value_or(level::err);
  }
  auto l_view = std::as_const(*g_reg()).view<const server_task_info>();
  for (auto &&[e, l_ptr] : l_view.each()) {
    if (l_ptr.id_ == l_task_handle.id_) {
      l_task_handle = l_ptr;
      break;
    }
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
  auto l_id = in_data->capture_->get("id");
  boost::uuids::uuid l_uuid{};
  {
    std::istringstream l_stream{l_id};
    l_stream >> l_uuid;
    if (l_stream.fail()) {
      in_error_code.assign(ERROR_INVALID_DATA, boost::system::system_category());
      BOOST_ASIO_ERROR_LOCATION(in_error_code);
      in_data->seed_error(boost::beast::http::status::bad_request, in_error_code);
      return;
    }
  }

  {
    for (auto &&[e, l_ptr] : g_reg()->view<server_task_info>().each()) {
      if (l_ptr.id_ == l_uuid) {
        g_reg()->destroy(e);
        break;
      }
    }
  }

  boost::beast::http::response<boost::beast::http::empty_body> l_response{
      boost::beast::http::status::ok, in_data->request_parser_->get().version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_data->request_parser_->get().keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.prepare_payload();
  in_data->seed(std::move(l_response));
  g_ctx().get<task_server>().erase_task(l_uuid);
}
void task_info::reg(doodle::http::http_route &in_route) {
  in_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task",
          session::make_http_reg_fun(boost::asio::bind_executor(g_thread(), &task_info::list_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task/{id}",
          session::make_http_reg_fun(boost::asio::bind_executor(g_thread(), &task_info::get_task))
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "v1/task/{id}/log",
          session::make_http_reg_fun(boost::asio::bind_executor(g_thread(), &task_info::get_task_logger))
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
