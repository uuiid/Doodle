//
// Created by TD on 2021/7/29.
//

#include "season.h"

#include <doodle_lib/gui/factory/attribute_factory_interface.h>


namespace doodle {
season::season()
    : 
      p_int(0) {
}
season::season(std::int32_t in_)
    :       p_int(in_) {
}

void season::set_season(std::int32_t in_) {
  p_int = in_;
}

std::int32_t season::get_season() const {
  return p_int;
}
std::string season::str() const {
  return fmt::format("seas_{}", p_int);
}
void season::attribute_widget(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(this);
}
bool season::operator<(const season& in_rhs) const {
  return p_int < in_rhs.p_int;
}
bool season::operator>(const season& in_rhs) const {
  return in_rhs < *this;
}
bool season::operator<=(const season& in_rhs) const {
  return !(in_rhs < *this);
}
bool season::operator>=(const season& in_rhs) const {
  return !(*this < in_rhs);
}
bool season::operator==(const season& in_rhs) const {
  return p_int == in_rhs.p_int;
}
bool season::operator!=(const season& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace doodle
