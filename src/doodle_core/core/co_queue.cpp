//
// Created by TD on 24-7-19.
//

#include "co_queue.h"

namespace doodle {
void awaitable_queue_limitation::awaitable_queue_impl::await_suspend(std::function<void()> in_handle) {
  {
    const std::lock_guard l{lock_};
    next_list_.emplace(in_handle);
  }
}

void awaitable_queue_limitation::awaitable_queue_impl::next() {
  const std::lock_guard l{lock_};
  if (!next_list_.empty()) {
    next_list_.front()();
    next_list_.pop();
    ++run_task_;
  }
}

void awaitable_queue_limitation::awaitable_queue_impl::maybe_invoke() {
  if (run_task_ >= limit_) return;
  while (run_task_ < limit_) next();
}
}  // namespace doodle