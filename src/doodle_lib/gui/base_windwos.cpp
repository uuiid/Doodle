//
// Created by TD on 2021/9/15.
//

#include "base_windwos.h"

#include <doodle_app.h>
#include <doodle_lib/gui/widget_register.h>
namespace doodle {
const string& base_widget::get_class_name() const{
  return p_class_name;
}
void base_widget::post_constructor() {
}

}  // namespace doodle
