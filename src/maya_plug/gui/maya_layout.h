//
// Created by TD on 2022/4/22.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/layout_window.h>
namespace doodle {
namespace maya_plug {

class maya_layout
    : public doodle::gui::layout_window {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  maya_layout();
  ~maya_layout();

  void update() override;
};

}  // namespace maya_plug
}  // namespace doodle
