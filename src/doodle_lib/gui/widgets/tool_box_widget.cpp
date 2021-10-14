//
// Created by TD on 2021/10/14.
//

#include "tool_box_widget.h"

#include <doodle_lib/gui/action/command.h>

namespace doodle {
tool_box_widget::tool_box_widget()
    : base_widget(),
      p_command_tool_ptr_() {
  p_class_name = "工具箱";
}

void tool_box_widget::frame_render() {
  if (p_command_tool_ptr_) {
    p_command_tool_ptr_->render();
  }
}

void tool_box_widget::set_tool_widget(const command_ptr& in_ptr) {
  p_command_tool_ptr_ = in_ptr;
}
}  // namespace doodle