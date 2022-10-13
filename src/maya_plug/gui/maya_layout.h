//
// Created by TD on 2022/4/22.
//
#pragma once

#include <doodle_app/gui/main_menu_bar.h>

namespace doodle {
namespace maya_plug {

class maya_layout
    : public doodle::gui::layout_window {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  maya_layout();
  virtual ~maya_layout();

  bool tick() override;
};


class maya_menu : public  doodle::gui::main_menu_bar{
 protected:
  void menu_windows() override;
  void menu_tool() override;
};
}  // namespace maya_plug
}  // namespace doodle
