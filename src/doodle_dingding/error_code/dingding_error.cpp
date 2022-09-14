//
// Created by TD on 2022/9/14.
//

#include "dingding_error.h"

namespace doodle::dingding {
const char* dingding_category::name() const noexcept {
  return "";
}
std::string dingding_category::message(int ev) const {
  return std::string();
}
char const* dingding_category::message(int ev, char* buffer, std::size_t len) const noexcept {
  return error_category::message(ev, buffer, len);
}
bool dingding_category::failed(int ev) const noexcept {
  return error_category::failed(ev);
}
bsys::error_condition dingding_category::default_error_condition(int ev) const noexcept {
  return error_category::default_error_condition(ev);
}
const bsys::error_category& dingding_category::get() {
  const static dingding_category l_doodle_category{};
  return l_doodle_category;
}
}  // namespace doodle::dingding
