//
// Created by TD on 2022/4/22.
//
#pragma once

#include <doodle_app/gui/layout_window_base.h>
#include <doodle_app/gui/main_menu_bar.h>

namespace doodle {
namespace maya_plug {

class maya_layout : public gui::details::layout_window_base {
 protected:
  virtual void layout(ImGuiID in_id, const ImVec2& in_size) override;

 public:
  maya_layout();
  virtual ~maya_layout();
};

class maya_menu : public doodle::gui::main_menu_bar {
 protected:
  void menu_windows() override;
  void menu_tool() override;
};
}  // namespace maya_plug
}  // namespace doodle
