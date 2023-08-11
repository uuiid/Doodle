//
// Created by td_main on 2023/8/10.
//

#include "computer.h"

#include <doodle_core/doodle_core_fwd.h>
namespace doodle {
namespace render_farm {
void computer::delay() {
  status_ = computer_status::idle;
  if (!timer_) {
    timer_ = std::make_shared<boost::asio::system_timer>(boost::asio::make_strand(g_io_context()));
  }
  timer_->expires_from_now(std::chrono::seconds(5));
  timer_->async_wait([this](auto e) {
    if (!e) status_ = computer_status::lost;
  });
}
}  // namespace render_farm
}  // namespace doodle