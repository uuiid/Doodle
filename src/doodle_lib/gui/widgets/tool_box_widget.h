//
// Created by TD on 2021/10/14.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>
namespace doodle {

class DOODLELIB_API tool_box_widget : public base_widget {
  command_ptr p_command_tool_ptr_;

 public:
  tool_box_widget();
  void frame_render() override;
  void set_tool_widget(const command_ptr& in_ptr);
};

}  // namespace doodle
