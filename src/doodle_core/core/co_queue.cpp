//
// Created by TD on 24-7-19.
//

#include "co_queue.h"

namespace doodle {
void awaitable_queue_limitation::awaitable_queue_impl::await_suspend(std::function<void()> in_handle) {
  ++await_task_count_;
  {
    const std::lock_guard l{lock_};
    next_list_.emplace(in_handle);
  }
}

void awaitable_queue_limitation::awaitable_queue_impl::next() {
  --await_task_count_;
  ++run_task_;
  const std::lock_guard l{lock_};
  next_list_.front()();
  next_list_.pop();
}

void awaitable_queue_limitation::awaitable_queue_impl::maybe_invoke() {
  if (await_task_count_ == 0) return;
  if (run_task_ >= limit_) return;
  while (await_task_count_ != 0 && run_task_ < limit_) next();
}
}  // namespace doodle