//
// Created by TD on 2021/5/7.
//

#include <DoodleLib/core/Name.h>
#include <PinYIn/convert.h>
namespace doodle {

Name::Name(std::string in_string)
    : p_string_(std::move(in_string)) {
  p_ENUS = convert::Get().toEn(p_string_);
}
Name::Name(std::string in_string, std::string in_ENUS)
    : p_string_(std::move(in_string)),
      p_ENUS(std::move(in_ENUS)) {
}
const std::string& Name::getName() const {
  return p_string_;
}
void Name::setName(const std::string& in_string) {
  p_string_ = in_string;
  p_ENUS    = convert::Get().toEn(p_string_);
}
const std::string& Name::getEnus() const {
  return p_ENUS;
}
void Name::setName(const std::string& in_string, const std::string& in_ENUS) {
  p_string_ = in_string;
  p_ENUS    = in_ENUS;
}

void Name::setEnus(const std::string& in_string) {
  p_ENUS = in_string;
}

}  // namespace doodle
