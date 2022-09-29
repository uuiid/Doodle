//
// Created by TD on 2022/4/27.
//

#include "main_proc_handle.h"

namespace doodle::gui {
main_proc_handle::main_proc_handle() = default;

main_proc_handle& main_proc_handle::get() {
  static main_proc_handle l_handle{};
  return l_handle;
}
main_proc_handle::~main_proc_handle() = default;
}  // namespace doodle::gui
