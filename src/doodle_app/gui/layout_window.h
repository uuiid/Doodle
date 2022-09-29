//
// Created by TD on 2022/4/13.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_app/gui/base/base_window.h>

namespace doodle::gui {

class DOODLELIB_API layout_window : public detail::windows_tick_interface {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  layout_window();
  virtual ~layout_window();

  bool tick();
};
}  // namespace doodle::gui
