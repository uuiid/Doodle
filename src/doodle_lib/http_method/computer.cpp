//
// Created by TD on 2024/2/26.
//

#include "computer.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"
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
  auto l_logger = in_handle->logger_;
  try {
    auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);
    if (!l_computer) {
      l_computer            = std::make_shared<computer_reg_data>();
      in_handle->user_data_ = l_computer;
      auto l_uuid           = in_handle->body_["user_id"].get<uuid>();
      if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<computer>(l_uuid); !l_list.empty())
        *l_computer->computer_data_ptr_ = l_list.front();
      l_computer->computer_data_ptr_->server_status_ = computer_status::free;
      l_computer->client                             = in_handle->client_;
      l_computer->computer_data_ptr_->uuid_id_       = l_uuid;
    }

    l_computer->computer_data_ptr_->client_status_ = in_handle->body_["state"].get<computer_status>();
    l_computer->computer_data_ptr_->server_status_ = in_handle->body_["state"].get<computer_status>();

    l_computer->computer_data_ptr_->name_          = in_handle->body_["host_name"].get<std::string>();
    l_computer->computer_data_ptr_->ip_            = in_handle->remote_endpoint_;
    if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_computer->computer_data_ptr_); !l_e)
      l_logger->log(log_loc(), level::err, "保存失败:{}", l_e.error());
    computer_reg_data_manager::get().reg(l_computer);
  } catch (...) {
    in_handle->logger_->error("设置状态出错 {}", boost::diagnostic_information(std::current_exception()));
  }

  co_return std::string{};
}

boost::asio::awaitable<std::string> web_logger_fun(http_websocket_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (!in_handle->user_data_) {
    l_logger->log(log_loc(), level::err, "用户数据为空");
    co_return std::string{};
  }
  auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);

  auto l_task     = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_)->task_info_;
  auto l_log_str  = in_handle->body_["msg"].get<std::string>();
  l_task->write_log(l_log_str);
  co_return std::string{};
}

boost::asio::awaitable<boost::beast::http::message_generator> list_computers(session_data_ptr in_handle) {
  std::vector<doodle::computer> l_computers = g_ctx().get<sqlite_database>().get_all<computer>();
  co_return in_handle->make_msg((nlohmann::json{} = l_computers).dump());
}

void reg_computer(const websocket_route_ptr& in_web_socket, const session_data_ptr& in_handle) {
  in_web_socket->reg("set_state", websocket_route::call_fun_type(web_set_tate_fun))
      .reg("logger", websocket_route::call_fun_type(web_logger_fun));

  in_web_socket->connect_close_signal([](const http_websocket_data_ptr& in_data) {
    auto l_computer = std::static_pointer_cast<computer_reg_data>(in_data->user_data_);
    if (l_computer) {
      computer_reg_data_manager::get().clear(l_computer);
      l_computer->computer_data_ptr_->server_status_ = computer_status::offline;
      l_computer->computer_data_ptr_->client_status_ = computer_status::offline;
      boost::asio::co_spawn(
          g_io_context(), g_ctx().get<sqlite_database>().install(l_computer->computer_data_ptr_), boost::asio::detached
      );
    }
  });
}
}  // namespace

void computer_reg(doodle::http::http_route& in_route) {
  in_route.reg(
      std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/computer", list_computers, reg_computer
      )
  );
}
}  // namespace doodle::http