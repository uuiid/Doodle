//
// Created by TD on 2022/4/8.
//

#include "base_window.h"
#include <doodle_lib/core/core_set.h>

namespace doodle::gui {

void window_panel::read_setting() {
  (*core_set::getSet().json_data)[title()].get_to(setting);
  //  core_set_init{}.read_setting(title(), setting);
}
void window_panel::save_setting() const {
  (*core_set::getSet().json_data)[title()] = setting;
//  core_set_init{}.save_setting(title(), setting);
}
}  // namespace doodle::gui
