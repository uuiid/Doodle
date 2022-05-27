//
// Created by TD on 2022/5/27.
//

#pragma once

#include <doodle_lib/gui/gui_ref/layout_window.h>
using namespace doodle;
class limited_layout : public doodle::gui::layout_window {
 public:
  void update(const chrono::system_clock::duration &in_duration,
              void *in_data) override;
};
