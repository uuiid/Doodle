//
// Created by teXiao on 2021/4/27.
//

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>

#include <boost/algorithm/string.hpp>

#include "core/core_help_impl.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include <treehh/tree.hh>

namespace doodle {

std::string assets::str() const { return p_path; }
void assets::set_path(const std::string& in_path) { p_path = in_path; }
const std::string& assets::get_path() const { return p_path; }

void assets::add_child(const entt::handle& in_child) {
  if (!in_child.any_of<assets>()) return;

  auto l_this_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_org_parent  = in_child.get<assets>().parent_;
  if (l_org_parent == l_this_handle) return;
  if (l_org_parent && l_org_parent.any_of<assets>() && l_org_parent.get<assets>().child_.contains(l_this_handle))
    return;
  if (child_.contains(in_child)) return;
  if (parent_ == in_child) return;

  child_.insert(in_child);
  in_child.patch<assets>().parent_ = l_this_handle;
  if (l_org_parent && l_org_parent.any_of<assets>()) l_org_parent.patch<assets>().child_.erase(in_child);
}
void assets::remove_child(const entt::handle& in_child) {
  if (child_.contains(in_child)) {
    child_.erase(in_child);
  }
}
void assets::set_parent(const entt::handle& in_parent) {
  auto l_this_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_old_parent  = parent_;
  if (child_.contains(in_parent)) return;

  if (in_parent && in_parent.any_of<assets>()) {
    if (l_old_parent && l_old_parent.any_of<assets>()) l_old_parent.patch<assets>().remove_child(l_this_handle);
    parent_ = in_parent;
    in_parent.patch<assets>().child_.insert(l_this_handle);
  } else {
    parent_ = entt::handle{};
    if (l_old_parent && l_old_parent.any_of<assets>()) l_old_parent.patch<assets>().remove_child(l_this_handle);
  }
}

std::set<entt::handle> assets::get_child() const { return child_; }

entt::handle assets::get_root() const {
  auto l_this_handle = entt::handle{*g_reg(), entt::to_entity(*g_reg(), *this)};
  auto l_parent      = parent_;
  while (l_parent && l_parent.any_of<assets>()) {
    l_this_handle = l_parent;
    l_parent      = l_parent.get<assets>().parent_;
  }
  return l_this_handle;
}

bool assets::operator<(const assets& in_rhs) const { return std::tie(p_path) < std::tie(in_rhs.p_path); }
bool assets::operator!=(const assets& in_rhs) const { return in_rhs.p_path != p_path; }

}  // namespace doodle
