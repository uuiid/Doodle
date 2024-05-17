//
// Created by TD on 2024/2/26.
//

#include "computer.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/server_task_info.h>

#include "doodle_lib/core/http/http_session_data.h"
#include "doodle_lib/core/http/http_websocket_data.h"
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/computer_reg_data.h>

namespace doodle::http {
void computer::websocket_route(const nlohmann::json &in_json, const http_websocket_data_ptr &in_handle) {
  auto l_logger   = in_handle->logger_;
  auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);

  switch (in_json["type"].get<computer_websocket_fun>()) {
    case computer_websocket_fun::set_state: {
      if (!in_json.contains("state") || !in_json["state"].is_string()) break;
      l_computer->computer_data_.client_status_ =
          magic_enum::enum_cast<doodle::computer_status>(in_json["state"].get<std::string>())
              .value_or(doodle::computer_status::unknown);

      if (in_json.contains("host_name") && in_json["host_name"].is_string()) {
        l_computer->computer_data_.name_ = in_json["host_name"].get<std::string>();
      }
      break;
    }
    case computer_websocket_fun::set_task: {
      if (!l_computer->task_info_) {
        l_logger->log(log_loc(), level::err, "task_info is null");
        break;
      }
      auto l_task     = l_computer->task_info_;
      l_task->status_ = magic_enum::enum_cast<server_task_info_status>(in_json["status"].get<std::string>())
                            .value_or(server_task_info_status::unknown);
      if (l_task->status_ == server_task_info_status::completed || l_task->status_ == server_task_info_status::failed) {
        l_task->end_time_ = std::chrono::system_clock::now();
      }
      l_computer->computer_data_.server_status_ = doodle::computer_status::free;
      break;
    }
    case computer_websocket_fun::logger: {
      if (!l_computer->task_info_) {
        l_logger->log(log_loc(), level::err, "task_info is null");
        break;
      }
      auto l_task = l_computer->task_info_;
      l_task->write_log(in_json["level"].get<level::level_enum>(), in_json["msg"].get<std::string>());
    }
  };
}
void computer::list_computers(boost::system::error_code in_error_code, const http_session_data_ptr &in_handle) {
  std::vector<doodle::computer> l_computers{};
  for (auto &&l_web_ : g_websocket_data_manager().get_list()) {
    if (auto l_computer = std::static_pointer_cast<computer_reg_data>(l_web_->user_data_); l_computer) {
      l_computers.emplace_back(l_computer->computer_data_);
    }
  }

  nlohmann::json l_json = l_computers;
  auto &l_req           = in_handle->request_parser_->get();
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, l_req.version()
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(l_req.keep_alive());
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  in_handle->seed(std::move(l_response));
}
void computer::reg_computer(boost::system::error_code in_error_code, const http_websocket_data_ptr &in_web_socket) {
  auto l_logger          = in_web_socket->logger_;
  auto l_remote_endpoint = boost::beast::get_lowest_layer(*in_web_socket->stream_).socket().remote_endpoint();
  l_logger->log(log_loc(), level::info, "注册计算机 {}", l_remote_endpoint.address().to_string());
  auto l_computer                           = std::make_shared<computer_reg_data>(doodle::computer{
      fmt::format("计算机 {}", l_remote_endpoint.address().to_string()), l_remote_endpoint.address().to_string()
  });
  l_computer->computer_data_.client_status_ = doodle::computer_status::free;
  in_web_socket->user_data_                 = l_computer;
  in_web_socket->on_message.connect(&computer::websocket_route);
}

void computer::reg(doodle::http::http_route &in_route) {
  in_route.reg(std::make_shared<http_function>(
      boost::beast::http::verb ::get, "v1/computer",
      session::make_http_reg_fun(
          boost::asio::bind_executor(g_io_context(), &computer::list_computers),
          boost::asio::bind_executor(g_io_context(), &computer::reg_computer)
      )
  ));
}
}  // namespace doodle::http