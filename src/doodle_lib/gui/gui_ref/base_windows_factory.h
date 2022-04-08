//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {
class base_window;
class DOODLELIB_API base_windows_factory {
 protected:
  base_window* window_{};

 public:
  base_windows_factory()               = default;
  virtual ~base_windows_factory()      = default;

  bool is_show();
  virtual base_window* make_windows()  = 0;
  virtual base_window* close_windows() = 0;
};

}  // namespace doodle::gui
