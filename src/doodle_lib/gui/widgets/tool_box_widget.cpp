//
// Created by TD on 2021/10/14.
//

#include "tool_box_widget.h"

#include <doodle_lib/gui/action/command.h>

namespace doodle {
tool_box_widget::tool_box_widget()
    : p_command_tool_ptr_() {
}

void tool_box_widget::set_tool_widget(const command_ptr& in_ptr) {
  p_command_tool_ptr_ = in_ptr;
}
void tool_box_widget::init() {
}
void tool_box_widget::succeeded() {
}
void tool_box_widget::failed() {
}
void tool_box_widget::aborted() {
}
void tool_box_widget::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  if (p_command_tool_ptr_) {
    p_command_tool_ptr_->render();
  }
}
}  // namespace doodle
