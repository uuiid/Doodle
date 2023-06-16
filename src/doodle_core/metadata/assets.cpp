//
// Created by teXiao on 2021/4/27.
//

#include <doodle_core/metadata/assets.h>

#include <boost/algorithm/string.hpp>

#include "core/core_help_impl.h"
#include "core/init_register.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
namespace doodle {

assets::assets() : p_path() {}

assets::assets(std::string in_name) : p_path(std::move(in_name)) {}

std::string assets::str() const { return p_path; }
void assets::set_path(const std::string& in_path) { p_path = in_path; }
const std::string& assets::get_path() const { return p_path; }

void assets::add_child(const entt::handle& in_child) {
  if (!in_child.any_of<assets>()) return;

  auto l_this_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_org_parent  = in_child.get<assets>().parent_;
  if (l_org_parent == l_this_handle) return;

  child_.insert(in_child);
  in_child.patch<assets>().parent_ = l_this_handle;
  if (l_org_parent && l_org_parent.any_of<assets>()) l_org_parent.patch<assets>().child_.erase(in_child);
}

bool assets::operator<(const assets& in_rhs) const { return std::tie(p_path) < std::tie(in_rhs.p_path); }
bool assets::operator!=(const assets& in_rhs) const { return in_rhs.p_path != p_path; }

DOODLE_REGISTER_BEGIN(assets) {
  entt::meta<assets>()
      .ctor<>()
      .ctor<std::string>()
      .data<&assets::p_path>("p_path"_hs)
      .func<&assets::set_path>("set_path"_hs)
      .func<&assets::get_path>("get_path"_hs);
}

}  // namespace doodle
