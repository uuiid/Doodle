//
// Created by TD on 2024/2/28.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::http {

class task_server {
  using timer_t     = boost::asio::steady_timer;
  using timer_t_ptr = std::shared_ptr<timer_t>;

  timer_t_ptr timer_ptr_{};
  logger_ptr logger_ptr_{};

  // 分配任务
  bool assign_task();

 public:
  task_server();
  ~task_server() = default;

  void run();
};

}  // namespace doodle::http