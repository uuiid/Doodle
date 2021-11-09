//
// Created by teXiao on 2021/4/27.
//

#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/metadata/assets.h>
#include <doodle_lib/metadata/metadata_factory.h>
#include <doodle_lib/pin_yin/convert.h>

BOOST_CLASS_EXPORT_IMPLEMENT(doodle::assets)
namespace doodle {
assets::assets()
    : p_name(),
      p_name_enus() {
}

assets::assets(std::string in_name)
    : p_name(std::move(in_name)),
      p_name_enus(convert::Get().toEn(p_name)) {
}

// Assets::~Assets() {
//   if (p_metadata_flctory_ptr_)
//     updata_db(p_metadata_flctory_ptr_);
// }

std::string assets::str() const {
  if (p_name_enus.empty())
    return convert::Get().toEn(p_name);
  return p_name_enus;
}
std::string assets::show_str() const {
  return p_name;
}

bool assets::operator<(const assets& in_rhs) const {
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_name) < std::tie(static_cast<const doodle::metadata&>(in_rhs), in_rhs.p_name);
  return std::tie(p_name) < std::tie(in_rhs.p_name);
}
bool assets::operator>(const assets& in_rhs) const {
  return in_rhs < *this;
}
bool assets::operator<=(const assets& in_rhs) const {
  return !(in_rhs < *this);
}
bool assets::operator>=(const assets& in_rhs) const {
  return !(*this < in_rhs);
}

const std::string& assets::get_name1() const {
  return p_name;
}
void assets::set_name1(const std::string& in_name) {
  p_name = in_name;
  if (p_name_enus.empty())
    p_name_enus = convert::Get().toEn(p_name);
}
const std::string& assets::get_name_enus() const {
  return p_name_enus;
}
void assets::set_name_enus(const std::string& in_nameEnus) {
  p_name_enus = in_nameEnus;
}
void assets::attribute_widget(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(this);
}
bool assets::operator==(const assets& in_rhs) const {
  return std::tie(p_name, p_name_enus) == std::tie(in_rhs.p_name, in_rhs.p_name_enus);
}
bool assets::operator!=(const assets& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace doodle
