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
    : p_path(),
      p_name_show_str() {
}

assets::assets(FSys::path in_name)
    : p_path(std::move(in_name)),
      p_name_show_str() {
  set_path_component();
}

void assets::set_path_component() {
  for (auto& in : p_path) {
    p_component.push_back(in.generic_string());
  }
}
std::string assets::str() const {
  return p_path.filename().generic_string();
}
void assets::set_path(const FSys::path& in_path) {
  p_path = in_path;
  set_path_component();
}
const FSys::path& assets::get_path() const {
  return p_path;
}

std::string assets::show_str() const {
  return p_name_show_str;
}

bool assets::operator<(const assets& in_rhs) const {
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_name) < std::tie(static_cast<const doodle::metadata&>(in_rhs), in_rhs.p_name);
  return std::tie(p_path) < std::tie(in_rhs.p_path);
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

void assets::attribute_widget(const attribute_factory_ptr& in_factoryPtr) {
  in_factoryPtr->show_attribute(this);
}
bool assets::operator==(const assets& in_rhs) const {
  return std::tie(p_path) == std::tie(in_rhs.p_path);
}
bool assets::operator!=(const assets& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace doodle
