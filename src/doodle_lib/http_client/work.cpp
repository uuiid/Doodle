//
// Created by TD on 2024/2/29.
//

#include "work.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/computer.h>

#include <doodle_lib/core/http/http_websocket_data.h>
#include <doodle_lib/http_method/computer.h>
namespace doodle::http {
void http_work::run(const std::string &in_server_address, std::uint16_t in_port) {
  timer_      = std::make_shared<timer>(g_io_context());
  signal_set_ = std::make_shared<signal_set>(g_io_context(), SIGINT, SIGTERM);
  signal_set_->async_wait([this](boost::system::error_code in_error_code, int in_signal) {
    if (in_error_code) {
      logger_->log(log_loc(), level::err, "signal_set error: {}", in_error_code);
      return;
    }
    if (in_signal == SIGINT || in_signal == SIGTERM) {
      if (handle_) {
        handle_.get<http_websocket_data>().do_close();
      }
    }
    app_base::Get().stop_app();
  });
  do_connect();
}
void http_work::do_connect() {
  if (!handle_) {
    handle_ = entt::handle{*g_reg(), g_reg()->create()};
    handle_.emplace<http_websocket_data>();
    logger_ = handle_.emplace<socket_logger>().logger_;
  }
  handle_.get<http_websocket_data>().async_connect(
      server_address_, "v1/computer", port_,
      [this](boost::system::error_code in_error_code) {
        if (in_error_code) {
          logger_->log(log_loc(), level::err, "连接失败 {}", in_error_code);
          do_wait();
          return;
        }
        handle_.get<http_websocket_data>().on_message.connect(
            std::bind(&http_work::read_task_info, this, std::placeholders::_1, std::placeholders::_2)
        );
        is_connect_ = true;
      }
  );
}
void http_work::do_wait() {
  logger_->log(log_loc(), level::info, "开始等待下一次心跳");
  timer_->expires_after(std::chrono::seconds{2});
  timer_->async_wait([this](boost::system::error_code in_error_code) {
    if (in_error_code == boost::asio::error::operation_aborted) {
      return;
    }
    if (in_error_code) {
      logger_->log(log_loc(), level::err, "on_wait error: {}", in_error_code);
      return;
    }
    if (!handle_ || !is_connect_) {
      do_connect();
      return;
    }
    send_state();
  });
}

void http_work::send_state() {
  if (!handle_) {
    return;
  }
  if (task_info_.task_info_.is_null()) {
    handle_.get<http_websocket_data>().seed(nlohmann::json{{"type", "set_state"}, {"state", computer_status::free}});
  } else {
    handle_.get<http_websocket_data>().seed(nlohmann::json{{"type", "set_state"}, {"state", computer_status::busy}});
  }
}
void http_work::read_task_info(const nlohmann::json &in_json, const entt::handle &in_handle) {
  if (!in_json.contains("id")) {
    logger_->log(log_loc(), level::err, "json parse error: {}", in_json.dump());
    return;
  }
  task_info_.task_id_   = in_json["id"].get<std::int32_t>();
  task_info_.task_info_ = in_json;
  send_state();
  run_task();
}

}  // namespace doodle::http