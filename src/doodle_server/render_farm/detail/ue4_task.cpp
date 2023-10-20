//
// Created by td_main on 2023/8/11.
//

#include "ue4_task.h"

#include <doodle_core/thread_pool/process_message.h>

#include <doodle_server/render_farm/detail/computer.h>
namespace doodle {
namespace render_farm {
namespace detail {
void ue4_task::set_meg() {
  auto& l_msg = self_handle_.get_or_emplace<process_message>();
  auto l_prj  = FSys::path{arg_.ProjectPath};
  l_msg.set_name(l_prj.filename().generic_string());
}
void ue4_task::assign_tasks() {
  auto l_view = g_reg()->view<computer>();

  for (auto&& [e, c] : l_view.each()) {
    if (c.status() == computer_status::idle) {
      c.run_task(self_handle_);
      computer_handle_ = entt::handle{*g_reg(), e};
      return;
    }
  }
}
bool ue4_task::is_assign() const { return computer_handle_ && computer_handle_.all_of<computer>(); }
void ue4_task::success() { self_handle_.get<process_message>().set_state(process_message::state::success); }
void ue4_task::fail() { computer_handle_ = entt::handle{}; }

bool ue4_task::is_success() const {
  return self_handle_.get<process_message>().get_state() != process_message::state::success;
}
}  // namespace detail
}  // namespace render_farm
}  // namespace doodle