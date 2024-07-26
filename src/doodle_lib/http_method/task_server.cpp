//
// Created by TD on 2024/2/28.
//

#include "task_server.h"

#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_websocket_client.h>
#include <doodle_lib/http_method/computer_reg_data.h>
namespace doodle::http {
task_server::task_server() : logger_ptr_(g_logger_ctrl().make_log("task_server")) {
  executor_  = boost::asio::make_strand(g_io_context());
  timer_ptr_ = std::make_shared<timer_t>(executor_);
}

void task_server::init() {
  for (auto&& [e, l_task] : g_reg()->view<doodle::server_task_info>().each()) {
    task_entity_map_[l_task.id_] = e;
    task_map_[l_task.id_]        = std::make_shared<doodle::server_task_info>(l_task);
  }
  clear_task();
}

void task_server::run() { boost::asio::co_spawn(executor_, async_run(), boost::asio::detached); }
boost::asio::awaitable<void> task_server::async_run() {
  if (is_running_) co_return;
  while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
    timer_ptr_->expires_after(1s);
    auto [l_ec] = co_await timer_ptr_->async_wait();
    if (l_ec) {
      logger_ptr_->log(log_loc(), level::warn, "timer_ptr_ error: {}", l_ec);
      co_return;
    }
    co_await assign_task();
    co_await confirm_task();
    clear_task();
    // if (std::none_of(task_map_.begin(), task_map_.end(), [](const auto& in_pair) {
    //       return in_pair.second->status_ == server_task_info_status::submitted;
    //     })) {
    //   break;
    // }
  }
}

boost::asio::awaitable<void> task_server::add_task(entt::entity in_entity) {
  co_await boost::asio::post(boost::asio::bind_executor(g_thread(), boost::asio::use_awaitable));
  auto l_task = g_reg()->get<doodle::server_task_info>(in_entity);
  co_await boost::asio::post(boost::asio::bind_executor(executor_, boost::asio::use_awaitable));

  task_entity_map_[l_task.id_] = in_entity;
  task_map_[l_task.id_]        = std::make_shared<doodle::server_task_info>(l_task);
}
boost::asio::awaitable<void> task_server::erase_task(const boost::uuids::uuid& in_id) {
  co_await boost::asio::post(boost::asio::bind_executor(g_thread(), boost::asio::use_awaitable));
  task_map_.erase(in_id);
  task_entity_map_.erase(in_id);
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

boost::asio::awaitable<void> task_server::confirm_task() {
  auto l_computers = computer_reg_data_manager::get().list();
  for (auto&& [id_, l_task] : task_map_) {
    if (l_task->status_ != server_task_info_status::assigned) continue;
    if (auto l_computer = std::find_if(
            l_computers.begin(), l_computers.end(),
            [l_task](const computer_reg_data_ptr& in_computer) { return in_computer->task_info_ == l_task; }
        );
        l_computer == l_computers.end()) {
      l_task->status_ = server_task_info_status::submitted;
      l_task->run_computer_.clear();
      l_task->run_time_ = std::chrono::system_clock::time_point{};
      {
        co_await boost::asio::post(boost::asio::bind_executor(g_thread(), boost::asio::use_awaitable));
        g_reg()->patch<doodle::server_task_info>(task_entity_map_[id_]) = *l_task;
        co_await boost::asio::post(boost::asio::bind_executor(executor_, boost::asio::use_awaitable));
      }
    }
  }
}

boost::asio::awaitable<void> task_server::assign_task() {
  auto l_computers = computer_reg_data_manager::get().list();
  for (auto&& [id_, l_task] : task_map_) {
    if (l_task->status_ != server_task_info_status::submitted) continue;

    for (auto& l_computer : l_computers) {
      if (l_computer->computer_data_.server_status_ != computer_status::free ||
          l_computer->computer_data_.client_status_ != computer_status::free)
        continue;

      l_computer->computer_data_.server_status_ = computer_status::busy;
      l_task->status_                           = server_task_info_status::assigned;
      l_task->run_computer_                     = l_computer->computer_data_.name_;
      l_task->run_time_                         = std::chrono::system_clock::now();
      l_task->run_computer_ip_                  = l_computer->computer_data_.ip_;
      {
        co_await boost::asio::post(boost::asio::bind_executor(g_thread(), boost::asio::use_awaitable));
        g_reg()->patch<doodle::server_task_info>(task_entity_map_[id_]) = *l_task;
        co_await boost::asio::post(boost::asio::bind_executor(executor_, boost::asio::use_awaitable));
      }
      nlohmann::json l_json{};

      l_json["type"]    = http_websocket_data_fun::run_task;
      l_json["id"]      = fmt::to_string(id_);
      l_json["exe"]     = l_task->exe_;
      l_json["command"] = l_task->command_;
      if (auto l_c = l_computer->client.lock(); l_c) {
        logger_ptr_->log(
            log_loc(), level::info, "分配任务 {}_{} 给 {}({})", l_task->name_, id_, l_computer->computer_data_.name_,
            l_computer->computer_data_.ip_
        );
        co_await l_c->async_write_websocket(l_json.dump());
        l_computer->task_info_        = l_task;
        l_computer->task_info_entity_ = task_entity_map_[id_];
      }
      break;
    }
  }
}

}  // namespace doodle::http