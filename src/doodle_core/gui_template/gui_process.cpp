//
// Created by TD on 2022/6/21.
//
#include "gui_process.h"

namespace doodle::detail {

void rear_adapter_t::operator()() {
  boost::asio::post(executor, [this, self_ = this->shared_from_this()]() {
    process();
    if (process.finished()) {
      if (this->next_value)
        (*this->next_value)();
    } else if (process.rejected()) {
      return;
    } else {
      (*this)();
    }
  });
}
rear_adapter_t::~rear_adapter_t() {
  if (process.alive()) {
    process.abort(true);
  }
}
rear_adapter_t::executor_type rear_adapter_t::get_executor() const noexcept {
  return this->executor;
}
void rear_adapter_t::updata_render() {
}
}  // namespace doodle::detail
void doodle::process_handy_tools::fail() {
  process_state_p = process_state::fail;
}
void doodle::process_handy_tools::succeed() {
  process_state_p = process_state::succeed;
}

// namespace doodle
