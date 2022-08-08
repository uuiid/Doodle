//
// Created by TD on 2021/5/7.
//

#include <doodle_core/metadata/user.h>
#include <pin_yin/convert.h>

namespace doodle {
user::user()
    : p_string_(),
      p_ENUS() {}

user::user(std::string in_string)
    : p_string_(std::move(in_string)) {
  p_ENUS = convert::Get().toEn(p_string_);
}
user::user(std::string in_string, std::string in_ENUS)
    : p_string_(std::move(in_string)),
      p_ENUS(std::move(in_ENUS)) {
}
const std::string& user::get_name() const {
  return p_string_;
}
void user::set_name(const std::string& in_string) {
  p_string_ = in_string;
  p_ENUS    = convert::Get().toEn(p_string_);
}

const std::string& user::get_enus() const {
  return p_ENUS;
}

}  // namespace doodle
