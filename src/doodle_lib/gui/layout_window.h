//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API layout_window {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  virtual void layout();
  virtual void init_windows();

 public:
  layout_window();
  virtual ~layout_window();

  bool render();
  const std::string& title() const;

  static void create_windows(windows&& in_windows);
};

}  // namespace doodle::gui
