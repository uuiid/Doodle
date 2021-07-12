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

void action_composited::set_class_name(const std::string& in_name) {
  action::p_name = in_name;
}

void action_composited::run(const MetadataPtr& in_data, const MetadataPtr& in_parent) {
  if (!action_indirect::sig_get_arg().value().is_cancel)
    return;
  auto& k_a = *p_action_list.back();
  k_a(in_data, in_parent);
}

actn_null::actn_null() {
  p_name = "取消";
}
}  // namespace doodle
