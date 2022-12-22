//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API layout_window : public detail::layout_tick_interface {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  layout_window();
  virtual ~layout_window() override;

  bool tick() override;
};

}  // namespace doodle

