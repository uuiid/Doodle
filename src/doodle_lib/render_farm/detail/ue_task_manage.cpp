//
// Created by td_main on 2023/8/16.
//

#include "ue_task_manage.h"

#include <doodle_lib/render_farm/detail/ue4_task.h>
namespace doodle {
namespace render_farm {

void ue_task_manage::run() {
  timer_ = std::make_shared<timer>(g_io_context());
  timer_->expires_from_now(1s);
  timer_->async_wait([this](const boost::system::error_code& ec) {
    for (auto&& [e, task] : g_reg()->view<detail::ue4_task>().each()) {
      task.assign_tasks();
    }
    this->run();
  });
}
}  // namespace render_farm
}  // namespace doodle