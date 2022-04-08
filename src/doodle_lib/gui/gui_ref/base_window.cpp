//
// Created by TD on 2022/4/8.
//

#include "base_window.h"
#include <doodle_lib/core/core_set.h>

namespace doodle::gui {

void window_panel::read_setting() {
  core_set::getSet();
}
void window_panel::save_setting() const {
}
}  // namespace doodle::gui
