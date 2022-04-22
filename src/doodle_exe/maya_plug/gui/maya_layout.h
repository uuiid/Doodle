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
 public:
  virtual void update(const chrono::system_clock::duration &in_duration, void *in_data) override;
};

}  // namespace maya_plug
}  // namespace doodle
