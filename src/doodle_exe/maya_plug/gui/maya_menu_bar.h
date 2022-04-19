//
// Created by TD on 2022/1/17.
//

#pragma once

#include <maya_plug_fwd.h>
#include <doodle_lib/gui/main_menu_bar.h>
namespace doodle::maya_plug {
class maya_menu_bar : public main_menu_bar {
 protected:
  void menu_windows() override;

 public:
};

}  // namespace doodle::maya_plug
