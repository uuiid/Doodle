//
// Created by td_main on 2023/8/11.
//

#include "ue4_task.h"

namespace doodle {
namespace render_farm {
namespace detail {
void ue4_task::set_meg() {
  auto& l_msg = self_handle_.get_or_emplace<process_message>();
  auto l_prj  = FSys::path{arg_.ProjectPath};
  l_msg.set_name(l_prj.filename().generic_string());
}
void ue4_task::assign_tasks() {}
}  // namespace detail
}  // namespace render_farm
}  // namespace doodle