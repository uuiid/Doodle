//
// Created by TD on 2021/6/17.
//

#include "action.h"

#include <DoodleLib/Exception/Exception.h>
#include <Logger/Logger.h>
namespace doodle {
action::action(std::any&& in_any)
    : p_any(std::move(in_any)),
      sig_get_input() {}
action::action()
    : p_any(),
      sig_get_input() {
}

bool action::is_accept(const _arg& in_any) {
  return false;
}
void action::set_any(std::any&& in_any) {
  p_any = std::move(in_any);
}
void action::set_any() {
  auto k_a = sig_get_input();
  if (!k_a) {
    DOODLE_LOG_DEBUG("没有获得值");
    throw DoodleError{"没有获得值"};
  }
  p_any = k_a.get();
}
std::string action::class_name() {
  return p_name;
}
void action::operator()(const MetadataPtr& in_data) {
  run(in_data);
}
}  // namespace doodle
