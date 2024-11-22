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

boost::asio::awaitable<void> list_tast_to(
    std::shared_ptr<computer> in_computer, std::shared_ptr<http_websocket_client> in_client
) {
  if (auto l_list = g_ctx().get<sqlite_database>().get_server_task_info(in_computer->uuid_id_);
      !l_list.empty() && in_client) {
    std::vector<uuid> l_ids{};
    for (const auto& l_item : l_list)
      if (l_item.status_ == server_task_info_status::submitted || l_item.status_ == server_task_info_status::assigned ||
          l_item.status_ == server_task_info_status::running)
        l_ids.emplace_back(l_item.uuid_id_);
    if (!l_ids.empty())
      co_await in_client->async_write_websocket(
          nlohmann::json{{"type", doodle_config::work_websocket_event::list_task}, {"ids", l_ids}}.dump()
      );
  }
}

boost::asio::awaitable<std::string> web_set_tate_fun(http_websocket_data_ptr in_handle) {
  auto l_logger   = in_handle->logger_;
  auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);
  try {
    if (!l_computer) {
      l_computer            = std::make_shared<computer_reg_data>();
      in_handle->user_data_ = l_computer;
      auto l_uuid           = in_handle->body_["user_id"].get<uuid>();
      if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<computer>(l_uuid); !l_list.empty())
        *l_computer->computer_data_ptr_ = l_list.front();
      l_computer->computer_data_ptr_->server_status_ = computer_status::free;
      l_computer->client                             = in_handle->client_;
      l_computer->computer_data_ptr_->uuid_id_       = l_uuid;
      boost::asio::co_spawn(
          g_io_context(), list_tast_to(l_computer->computer_data_ptr_, in_handle->client_.lock()),
          boost ::asio::consign(boost::asio::detached, l_computer->computer_data_ptr_, in_handle->client_.lock())
      );
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

  auto l_id       = in_handle->body_["id"].get<uuid>();
  auto l_path = core_set::get_set().get_cache_root() / server_task_info::logger_category / fmt::format("{}.log", l_id);
  FSys::ofstream{l_path, std::ios::binary | std::ios::out | std::ios::app } << in_handle->body_["msg"].get<std::string>();
  co_return std::string{};
}

boost::asio::awaitable<std::string> web_set_task_state_fun(http_websocket_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;
  if (!in_handle->user_data_) {
    l_logger->log(log_loc(), level::err, "用户数据为空");
    co_return std::string{};
  }
  auto l_computer = std::static_pointer_cast<computer_reg_data>(in_handle->user_data_);
  auto l_id       = in_handle->body_["id"].get<uuid>();
  if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<server_task_info>(l_id); !l_list.empty()) {
    auto l_task     = std::make_shared<server_task_info>();
    *l_task         = l_list.front();
    l_task->status_ = in_handle->body_["state"].get<server_task_info_status>();
    switch (l_task->status_) {
      case server_task_info_status::running:
        l_task->run_time_ = std::chrono::system_clock::now();
        break;
      case server_task_info_status::completed:
      case server_task_info_status::canceled:
      case server_task_info_status::failed:
        l_task->end_time_ = std::chrono::system_clock::now();
        break;
      default:
        break;
    }
    if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_task); !l_e)
      l_logger->log(log_loc(), level::err, "保存失败:{}", l_e.error());
  }
  co_return std::string{};
}

boost::asio::awaitable<boost::beast::http::message_generator> list_computers(session_data_ptr in_handle) {
  std::vector<doodle::computer> l_computers = g_ctx().get<sqlite_database>().get_all<computer>();
  co_return in_handle->make_msg((nlohmann::json{} = l_computers).dump());
}

void reg_computer(const websocket_route_ptr& in_web_socket, const session_data_ptr& in_handle) {
  in_web_socket
      ->reg(
          std::string{doodle_config::server_websocket_event::set_state},
          websocket_route::call_fun_type(web_set_tate_fun)
      )
      .reg(std::string{doodle_config::server_websocket_event::logger}, websocket_route::call_fun_type(web_logger_fun))
      .reg(
          std::string{doodle_config::server_websocket_event::set_task_state},
          websocket_route::call_fun_type(web_set_task_state_fun)
      );

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
  default_logger_raw()->info("任务日志目录 {}", core_set::get_set().get_cache_root(server_task_info::logger_category));
  in_route.reg(
      std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/computer", list_computers, reg_computer
      )
  );
}
}  // namespace doodle::http