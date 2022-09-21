//
// Created by TD on 2022/4/13.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

#include <doodle_lib/gui/strand_gui.h>
namespace doodle::gui {

class DOODLELIB_API layout_window {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  layout_window();
  virtual ~layout_window();

  bool tick();
};
}  // namespace doodle::gui
