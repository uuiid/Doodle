//
// Created by TD on 2024/2/28.
//

#include "task_server.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_websocket_data.h>
namespace doodle::http {
task_server::task_server()
    : logger_ptr_(g_logger_ctrl().make_log("task_server")), timer_ptr_(std::make_shared<timer_t>(g_io_context())) {}

void task_server::run() {
  if (!assign_task()) return;

  timer_ptr_->expires_after(std::chrono::seconds(1));
  timer_ptr_->async_wait([this](boost::system::error_code ec) {
    if (ec) {
      logger_ptr_->log(log_loc(), level::warn, "timer_ptr_ error: {}", ec);
      return;
    }
    run();
  });
}

bool task_server::assign_task() {
  bool has_task = false;
  for (auto&& [l_e, l_task] : g_reg()->view<server_task_info>().each()) {
    if (l_task.status_ != server_task_info_status::submitted) continue;
    has_task = true;

    for (auto&& [l_e2, l_computer, l_websocket] : g_reg()->view<computer, http_websocket_data>().each()) {
      if (l_computer.server_status_ != computer_status::free && l_computer.client_status_ != computer_status::free)
        continue;

      l_computer.server_status_ = computer_status::busy;
      l_task.status_            = server_task_info_status::assigned;
      l_task.run_computer_      = l_computer.name_;
      l_task.run_time_          = std::chrono::system_clock::now();
      l_task.run_computer_ip_   = l_computer.ip_;

      nlohmann::json l_json{};

      l_json["id"]   = l_e;
      l_json["data"] = l_task.data_;
      l_websocket.write_queue_.emplace(l_json.dump());
      logger_ptr_->log(
          log_loc(), level::info, "分配任务 {}_{} 给 {}({})", l_task.name_, l_e, l_computer.name_, l_computer.ip_
      );
      entt::handle l_computer_handle{*g_reg(), l_e2};
      l_computer_handle.emplace_or_replace<task_ref>(entt::handle{*g_reg(), l_e});
    }
  }
  return has_task;
}

}  // namespace doodle::http