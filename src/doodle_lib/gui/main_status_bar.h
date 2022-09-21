//
// Created by TD on 2022/1/14.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API main_status_bar {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  main_status_bar();
  virtual ~main_status_bar();

  [[maybe_unused]] void update();
};
}  // namespace doodle
