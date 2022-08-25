//
// Created by TD on 2022/7/11.
//

#include "process_bounded.h"
namespace doodle {
namespace process_bounded_ns {
process_bounded_server::process_bounded_server(
    boost::asio::execution_context& context
)
    : execution_context_service_base(context),
      mutex_() {
}
void process_bounded_server::shutdown() {
  std::lock_guard l_g{mutex_};

  impl_list_->handlers.clear();
  impl_list_->handlers_next.clear();
}
process_bounded_server::implementation_type
process_bounded_server::create_implementation() {
  std::lock_guard l_g{mutex_};
  if (!impl_list_) {
    impl_list_           = std::make_shared<strand_impl>();
    impl_list_->service_ = this;
    impl_list_->mutex_   = &this->mutex_;
  }
  return impl_list_;
}
void process_bounded_server::stop(const process_bounded_server::implementation_type& in_impl) {
}
void process_bounded_server::loop_one() {
}
}  // namespace process_bounded_ns
}  // namespace doodle
