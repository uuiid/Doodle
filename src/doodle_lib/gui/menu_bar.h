//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_app/gui/main_menu_bar.h>
namespace doodle::gui {
class DOODLELIB_API menu_bar : public main_menu_bar {
 public:
 static void message(const std::string&in_m);
 protected:
  void menu_windows() override;
  void menu_tool() override;
};
}  // namespace doodle::gui
