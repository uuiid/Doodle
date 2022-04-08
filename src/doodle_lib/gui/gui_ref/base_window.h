//
// Created by TD on 2022/4/8.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API base_window {
 public:
  base_window()               = default;
  virtual ~base_window()      = default;

  virtual std::string title() = 0;
  virtual void init()         = 0;
  virtual void succeeded()    = 0;
  virtual void failed()       = 0;
  virtual void aborted()      = 0;
  virtual void update(
      const chrono::system_clock::duration& in_duration,
      void* in_data) = 0;
};



}  // namespace doodle::gui
