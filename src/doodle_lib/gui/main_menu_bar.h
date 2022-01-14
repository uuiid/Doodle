//
// Created by TD on 2022/1/14.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle {
class DOODLELIB_API main_menu_bar : public process_t<main_menu_bar> {
 public:
  using base_type = process_t<main_menu_bar>;

  main_menu_bar();
  ~main_menu_bar() override;

};
}  // namespace doodle
