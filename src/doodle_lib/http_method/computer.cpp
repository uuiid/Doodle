//
// Created by TD on 2024/2/26.
//

#include "computer.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/http/websocket_route.h>
#include <doodle_lib/http_method/computer_reg_data.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<std::string> web_set_tate_fun(http_websocket_data_ptr in_handle) {
  auto l_logger   = in_handle->logger_;
  auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);

  if (!l_computer) {
    l_computer                                = std::make_shared<computer_reg_data>();
    in_handle->user_data_                     = l_computer;
    l_computer->computer_data_.server_status_ = computer_status::free;
    computer_reg_data_manager::get().reg(l_computer);
  }

  if (!in_handle->body_.contains("state") || !in_handle->body_["state"].is_string()) co_return std::string{};
  l_computer->computer_data_.client_status_ =
      magic_enum::enum_cast<doodle::computer_status>(in_handle->body_["state"].get<std::string>())
          .value_or(doodle::computer_status::unknown);

  if (in_handle->body_.contains("host_name") && in_handle->body_["host_name"].is_string()) {
    l_computer->computer_data_.name_ = in_handle->body_["host_name"].get<std::string>();
  }
  l_computer->computer_data_.ip_ = in_handle->remote_endpoint_;

  co_return std::string{};
}

boost::asio::awaitable<std::string> web_logger_fun(http_websocket_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (!in_handle->user_data_) {
    l_logger->log(log_loc(), level::err, "用户数据为空");
    co_return std::string{};
  }
  auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);
  if (!l_computer->task_info_) {
    l_logger->log(log_loc(), level::err, "task_info is null");
    co_return std::string{};
  }
  auto l_task = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_)->task_info_;
  l_task->write_log(in_handle->body_["level"].get<level::level_enum>(), in_handle->body_["msg"].get<std::string>());
  co_return std::string{};
}

boost::asio::awaitable<std::string> web_set_task_fun(http_websocket_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (!in_handle->user_data_) {
    l_logger->log(log_loc(), level::err, "用户数据为空");
    co_return std::string{};
  }

  auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);
  if (!l_computer->task_info_) {
    l_logger->log(log_loc(), level::err, "task_info is null");
    co_return std::string{};
  }
  auto l_task     = l_computer->task_info_;
  l_task->status_ = magic_enum::enum_cast<server_task_info_status>(in_handle->body_["status"].get<std::string>())
                        .value_or(server_task_info_status::unknown);
  if (l_task->status_ == server_task_info_status::completed || l_task->status_ == server_task_info_status::failed) {
    l_task->end_time_ = std::chrono::system_clock::now();
  }
  {
    g_reg()->patch<doodle::server_task_info>(
        l_computer->task_info_entity_, [l_task](doodle::server_task_info& in_task) { in_task = *l_task; }
    );
  }
  l_computer->computer_data_.server_status_ = doodle::computer_status::free;
  co_return std::string{};
}

boost::asio::awaitable<boost::beast::http::message_generator> list_computers(session_data_ptr in_handle) {
  std::vector<doodle::computer> l_computers{};
  for (auto&& l_web_ : computer_reg_data_manager::get().list()) {
    l_computers.emplace_back(l_web_->computer_data_);
  }

  nlohmann::json l_json = l_computers;
  boost::beast::http::response<boost::beast::http::string_body> l_response{
      boost::beast::http::status::ok, in_handle->version_
  };
  l_response.result(boost::beast::http::status::ok);
  l_response.keep_alive(in_handle->keep_alive_);
  l_response.set(boost::beast::http::field::content_type, "application/json");
  l_response.body() = l_json.dump();
  l_response.prepare_payload();
  co_return std::move(l_response);
}

void reg_computer(const websocket_route_ptr& in_web_socket) {
  in_web_socket->reg("set_state", std::make_shared<websocket_route::call_fun_type>(web_set_tate_fun))
      .reg("logger", std::make_shared<websocket_route::call_fun_type>(web_logger_fun))
      .reg("set_task", std::make_shared<websocket_route::call_fun_type>(web_set_task_fun));
}
}  // namespace

void computer_reg(doodle::http::http_route& in_route) {
  in_route.reg(
      std::make_shared<http_function>(boost::beast::http::verb::get, "v1/computer", list_computers, reg_computer)
  );
}
}  // namespace doodle::http