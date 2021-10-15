//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_lib/gui/main_windwos.h>

namespace doodle::maya_plug {
class maya_windwos : public main_windows {
  void main_menu_tool() override;

 public:
  maya_windwos();
  void frame_render() override;
};

}  // namespace doodle::maya_plug
