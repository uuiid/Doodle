//
// Created by td_main on 2023/8/10.
//

#include "computer.h"

#include <doodle_core/doodle_core_fwd.h>

#include <magic_enum.hpp>
namespace doodle {
namespace render_farm {
void computer::delay(computer_status in_status) {
  status_ = in_status;

  if (!timer_) {
    timer_ = std::make_shared<boost::asio::system_timer>(boost::asio::make_strand(g_io_context()));
  }
  timer_->expires_from_now(std::chrono::seconds(5));
  timer_->async_wait([this](auto e) {
    if (!e && status_ == computer_status::lost) {
      status_ = computer_status::lost;
    }
  });
}
void computer::delay(const std::string& in_str) {
  auto l_status = magic_enum::enum_cast<computer_status>(in_str);
  delay(l_status.value_or(computer_status::idle));
}
}  // namespace render_farm
}  // namespace doodle