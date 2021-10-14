//
// Created by TD on 2021/10/14.
//
#pragma once
#include <doodle_lib/gui/base_windwos.h>

namespace doodle::maya_plug {
class maya_tool_widget : public base_widget {
  
 public:
  maya_tool_widget();
  void frame_render() override;
};

}  // namespace doodle::maya_plug
