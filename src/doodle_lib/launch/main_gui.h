//
// Created by TD on 2023/12/28.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <argh.h>

namespace doodle::launch {

class main_gui_launcher_t {
 public:
  main_gui_launcher_t()  = default;
  ~main_gui_launcher_t() = default;

  bool operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector);
};

}  // namespace doodle::launch
