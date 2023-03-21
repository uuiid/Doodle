//
// Created by TD on 2022/9/19.
//
#include "exception.h"

namespace doodle::maya_plug {
const char* maya_category::name() const noexcept { return "maya"; }
std::string maya_category::message(int ev) const { return {}; }

bsys::error_condition maya_category::default_error_condition(int ev) const noexcept {
  return error_category::default_error_condition(ev);
}
const bsys::error_category& maya_category::get() {
  static maya_category install{};
  return install;
}
}  // namespace doodle::maya_plug
