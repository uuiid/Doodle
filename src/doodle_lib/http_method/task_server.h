//
// Created by TD on 2024/2/28.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class server_task_info;
}
namespace doodle::http {

class task_server {
  using executor_type = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;
  using timer_t       = executor_type::as_default_on_t<boost::asio::steady_timer>;
  using timer_t_ptr   = std::shared_ptr<timer_t>;

  timer_t_ptr timer_ptr_{};
  logger_ptr logger_ptr_{};
  boost::asio::any_io_executor executor_{};

  bool is_running_ = false;
  // 运行守卫
  struct run_guard_t {
    explicit run_guard_t(task_server* in_ptr) : ptr_(in_ptr) { ptr_->is_running_ = true; }
    ~run_guard_t() { ptr_->is_running_ = false; }
    task_server* ptr_;
  };

  // 分配任务
  boost::asio::awaitable<void> assign_task();
  // 此处清理已经完成的任务, 没有指定调用线程
  void clear_task();
  // 确认计算机正在运行的任务, 清除已分配, 但是计算机掉线的任务
  boost::asio::awaitable<void> confirm_task();
  boost::asio::awaitable<void> async_run();
  std::map<boost::uuids::uuid, std::shared_ptr<doodle::server_task_info>> task_map_{};  // 任务列表
  std::map<boost::uuids::uuid, entt::entity> task_entity_map_{};                        // 任务实体列表
 public:
  task_server();
  ~task_server() = default;

  // 此处可进行多线程调用
  void run();
  boost::asio::awaitable<void> add_task(entt::entity in_entity);
  boost::asio::awaitable<void> erase_task(const boost::uuids::uuid& in_id);

  // 初始化必须在主线程调用 
  void init();
};

}  // namespace doodle::http