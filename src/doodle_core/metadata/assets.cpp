//
// Created by teXiao on 2021/4/27.
//

#include <doodle_core/metadata/assets.h>

#include "core/init_register.h"

namespace doodle {

assets::assets() : p_path() {}

assets::assets(FSys::path in_name) : p_path(std::move(in_name)) { set_path_component(); }

void assets::set_path_component() {
  p_component.clear();
  for (auto& in : p_path) {
    p_component.push_back(in.generic_string());
  }
}
std::string assets::str() const { return p_path.filename().generic_string(); }
void assets::set_path(const FSys::path& in_path) {
  p_path = in_path;
  set_path_component();
}
const FSys::path& assets::get_path() const { return p_path; }

bool assets::operator<(const assets& in_rhs) const {
  //  return std::tie(static_cast<const doodle::metadata&>(*this), p_name) < std::tie(static_cast<const
  //  doodle::metadata&>(in_rhs), in_rhs.p_name);
  return std::tie(p_path) < std::tie(in_rhs.p_path);
}
bool assets::operator>(const assets& in_rhs) const { return in_rhs < *this; }
bool assets::operator<=(const assets& in_rhs) const { return !(in_rhs < *this); }
bool assets::operator>=(const assets& in_rhs) const { return !(*this < in_rhs); }

bool assets::operator==(const assets& in_rhs) const { return std::tie(p_path) == std::tie(in_rhs.p_path); }
bool assets::operator!=(const assets& in_rhs) const { return !(in_rhs == *this); }

DOODLE_REGISTER_BEGIN(assets) {
  entt::meta<assets>()
      .ctor<>()
      .ctor<FSys::path>()
      .data<&assets::p_path>("p_path"_hs)
      .func<&assets::get_path_component>("get_path_component"_hs)
      .func<&assets::set_path>("set_path"_hs)
      .func<&assets::get_path>("get_path"_hs);
}

}  // namespace doodle
