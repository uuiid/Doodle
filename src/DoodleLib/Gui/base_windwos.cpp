//
// Created by TD on 2021/9/15.
//

#include "base_windwos.h"
namespace doodle {
const string& base_widget::get_class_name() {
  return p_class_name;
}

attribute_factory_ptr metadata_widget::get_factory() {
  return p_factory;
}

}  // namespace doodle
