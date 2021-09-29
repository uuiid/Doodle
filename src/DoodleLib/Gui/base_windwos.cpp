//
// Created by TD on 2021/9/15.
//

#include "base_windwos.h"

#include <DoodleLib/Gui/widget_register.h>
#include <doodle_app.h>
namespace doodle {
const string& base_widget::get_class_name() {
  return p_class_name;
}
void base_widget::post_constructor() {
  doodle_app::Get()->get_register()->get().insert(std::make_pair(std::type_index{typeid(*this)}, this->weak_from_this()));
}

attribute_factory_ptr metadata_widget::get_factory() {
  return p_factory;
}

}  // namespace doodle
