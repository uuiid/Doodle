//
// Created by TD on 2022/1/14.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API main_status_bar : public process_t<main_status_bar> {
 private:
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  main_status_bar();
  ~main_status_bar() override;

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
};
}  // namespace doodle
