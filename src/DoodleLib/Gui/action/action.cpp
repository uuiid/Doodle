//
// Created by TD on 2021/6/17.
//

#include "action.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
namespace doodle {
action::action() {
}

std::string action::class_name() {
  return p_name;
}
void action::operator()(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  run(in_data, in_parent);
}

actn_null::actn_null() {
  p_name = "取消";
}
}  // namespace doodle
