//
// Created by TD on 2023/12/8.
//

#include "process_callback.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/thread_pool/process_message.h>
namespace doodle {
process_callback &process_callback::set_handle(const entt::handle &in_handle) {
  if (!in_handle.any_of<process_message>()) {
    throw_exception(doodle_error{"缺失消息组件"});
  }

  handle_ = in_handle;
  return *this;
}

void process_callback::operator()(boost::system::error_code in_error_code) const {
  if (in_error_code) {
    handle_.get<process_message>().set_state(process_message::state::fail);
  } else {
    handle_.get<process_message>().set_state(process_message::state::success);
  }
}

}  // namespace doodle