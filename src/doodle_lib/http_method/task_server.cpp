//
// Created by TD on 2024/2/28.
//

#include "task_server.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/http_method/computer_reg_data.h>
namespace doodle::http {
task_server::task_server()
    : logger_ptr_(g_logger_ctrl().make_log("task_server")), timer_ptr_(std::make_shared<timer_t>(g_io_context())) {}

void task_server::init() {
  for (auto&& [e, l_task] : g_reg()->view<doodle::server_task_info>().each()) {
    task_entity_map_[l_task.id_] = e;
    task_map_[l_task.id_]        = std::make_shared<doodle::server_task_info>(l_task);
  }
  clear_task();
}

void task_server::run() {
  boost::asio::post(g_io_context(), [this] {
    if (is_running_) return;
    begin_assign_task();
  });
}

void task_server::begin_assign_task() {
  timer_ptr_->expires_after(std::chrono::seconds(1));
  timer_ptr_->async_wait([this, l_g = run_guard_t{this}](boost::system::error_code ec) {
    if (ec) {
      logger_ptr_->log(log_loc(), level::warn, "timer_ptr_ error: {}", ec);
      return;
    }
    assign_task();
    confirm_task();
    clear_task();
    if (std::any_of(task_map_.begin(), task_map_.end(), [](const auto& in_pair) {
          return in_pair.second->status_ == server_task_info_status::submitted;
        })) {
      run();
    }
  });
}
void task_server::add_task(entt::entity in_entity) {
  boost::asio::post(g_io_context(), [this, in_entity] {
    auto l_task                  = g_reg()->get<doodle::server_task_info>(in_entity);
    task_entity_map_[l_task.id_] = in_entity;
    task_map_[l_task.id_]        = std::make_shared<doodle::server_task_info>(l_task);
  });
}
void task_server::erase_task(const boost::uuids::uuid& in_id) {
  boost::asio::post(g_io_context(), [this, in_id = std::move(in_id)] {
    task_map_.erase(in_id);
    task_entity_map_.erase(in_id);
  });
}

void task_server::clear_task() {
  for (auto it = task_map_.begin(); it != task_map_.end();) {
    if (it->second->status_ == server_task_info_status::completed) {
      task_entity_map_.erase(it->first);
      it = task_map_.erase(it);
    } else {
      ++it;
    }
  }
}

void task_server::confirm_task() {
  // auto l_computers = g_websocket_data_manager().get_list();
  // for (auto&& [id_, l_task] : task_map_) {
  //   if (l_task->status_ != server_task_info_status::assigned) continue;
  //   if (auto l_computer = std::find_if(
  //           l_computers.begin(), l_computers.end(),
  //           [l_task](const auto& in_computer) {
  //             if (auto l_computer_data = std::static_pointer_cast<computer_reg_data>(in_computer->user_data_);
  //                 l_computer_data) {
  //               return l_computer_data->task_info_ == l_task;
  //             }
  //             return false;
  //           }
  //       );
  //       l_computer == l_computers.end()) {
  //     l_task->status_ = server_task_info_status::submitted;
  //     l_task->run_computer_.clear();
  //     l_task->run_time_ = std::chrono::system_clock::time_point{};
  //     {
  //       g_reg()->patch<doodle::server_task_info>(task_entity_map_[id_], [l_task](doodle::server_task_info& in_task) {
  //         in_task = *l_task;
  //       });
  //     }
  //   }
  // }
}

void task_server::assign_task() {
  // auto l_computers = g_websocket_data_manager().get_list();
  // for (auto&& [id_, l_task] : task_map_) {
  //   if (l_task->status_ != server_task_info_status::submitted) continue;
  //
  //   for (auto& l_computer : l_computers) {
  //     if (auto l_computer_data = std::static_pointer_cast<computer_reg_data>(l_computer->user_data_); l_computer_data) {
  //       if (l_computer_data->computer_data_.server_status_ != computer_status::free ||
  //           l_computer_data->computer_data_.client_status_ != computer_status::free)
  //         continue;
  //
  //       l_computer_data->computer_data_.server_status_ = computer_status::busy;
  //       l_task->status_                                = server_task_info_status::assigned;
  //       l_task->run_computer_                          = l_computer_data->computer_data_.name_;
  //       l_task->run_time_                              = std::chrono::system_clock::now();
  //       l_task->run_computer_ip_                       = l_computer_data->computer_data_.ip_;
  //       {
  //         g_reg()->patch<doodle::server_task_info>(task_entity_map_[id_], [l_task](doodle::server_task_info& in_task) {
  //           in_task = *l_task;
  //         });
  //       }
  //       nlohmann::json l_json{};
  //
  //       l_json["type"]    = http_websocket_data_fun::run_task;
  //       l_json["id"]      = fmt::to_string(id_);
  //       l_json["exe"]     = l_task->exe_;
  //       l_json["command"] = l_task->command_;
  //       l_computer->seed(l_json);
  //       logger_ptr_->log(
  //           log_loc(), level::info, "分配任务 {}_{} 给 {}({})", l_task->name_, id_,
  //           l_computer_data->computer_data_.name_, l_computer_data->computer_data_.ip_
  //       );
  //       l_computer_data->task_info_ = l_task;
  //       l_computer_data->task_info_entity_ = task_entity_map_[id_];
  //       break;
  //     }
  //   }
  // }
}

}  // namespace doodle::http