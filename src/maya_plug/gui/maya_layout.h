//
// Created by TD on 2022/4/22.
//
#pragma once

// #include <doodle_app/gui/layout_window.h>
#include <doodle_app/gui/main_menu_bar.h>

namespace doodle {
namespace maya_plug {

class maya_layout {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  virtual void layout(ImGuiID in_id, const ImVec2& in_size);
  virtual void init_windows();

 public:
  maya_layout();
  virtual ~maya_layout();

  bool render();
};

class maya_menu : public doodle::gui::main_menu_bar {
 protected:
  void menu_windows() override;
  void menu_tool() override;
};
}  // namespace maya_plug
}  // namespace doodle
