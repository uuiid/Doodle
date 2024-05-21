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
  using timer_t     = boost::asio::steady_timer;
  using timer_t_ptr = std::shared_ptr<timer_t>;

  timer_t_ptr timer_ptr_{};
  logger_ptr logger_ptr_{};

  // 分配任务
  bool assign_task();

  bool is_running_ = false;
  // 运行守卫
  struct run_guard_t {
    explicit run_guard_t(task_server* in_ptr) : ptr_(in_ptr) { ptr_->is_running_ = true; }
    ~run_guard_t() { ptr_->is_running_ = false; }
    task_server* ptr_;
  };

  void begin_assign_task();
  // 此处清理已经完成的任务, 没有指定调用线程
  void clear_task();

  std::map<boost::uuids::uuid, std::shared_ptr<doodle::server_task_info>> task_map_{};  // 任务列表
 public:
  task_server();
  ~task_server() = default;

  // 此处可进行多线程调用
  void run();
  void add_task(doodle::server_task_info in_task);
  void erase_task(const boost::uuids::uuid& in_id);

  // 初始化必须在主线程调用, 并且必须初始化数据库
  void init(pooled_connection& in_conn);
};

}  // namespace doodle::http