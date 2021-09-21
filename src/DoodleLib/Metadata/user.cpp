//
// Created by TD on 2021/5/7.
//

#include <Metadata/user.h>
#include <PinYin/convert.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::user)
namespace doodle {

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

void user::set_name(const std::string& in_string, const std::string& in_ENUS) {
  p_string_ = in_string;
  p_ENUS    = in_ENUS;
}

const std::string& user::get_enus() const {
  return p_ENUS;
}

void user::set_enus(const std::string& in_string) {
  p_ENUS = in_string;
}
const user::time_pair_list& user::get_time_work_list() const {
  return p_time_work;
}
user::time_pair_list& user::get_time_work_list() {
  return p_time_work;
}
const user::time_pair_list& user::get_time_rest_list() const {
  return p_time_rest;
}
user::time_pair_list& user::get_time_rest_list() {
  return p_time_rest;
}

}  // namespace doodle
