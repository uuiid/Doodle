//
// Created by td_main on 2023/8/16.
//

#include "computer_manage.h"

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/render_farm/detail/computer.h>
namespace doodle {
namespace render_farm {
void computer_manage::run() {
  timer_ = std::make_shared<timer>(g_io_context());
  timer_->expires_from_now(3s);
  timer_->async_wait([this](const boost::system::error_code& ec) {
    if (ec == boost::asio::error::operation_aborted) {
      return;
    }
    for (auto&& [e, computer] : g_reg()->view<computer>().each()) {
      if (computer.status() == computer_status::idle) {
        computer.set_status(computer_status::lost);
      }
    }
    this->run();
  });
}
}  // namespace render_farm
}  // namespace doodle