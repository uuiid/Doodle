//
// Created by TD on 2023/12/20.
//
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/main_map.h>

#include <core/scan_assets/base.h>

namespace doodle::details {

entt::handle scan_category_data_t::get_project_handle() const {
  auto l_proect_view = g_reg()->view<project, assets>().each();
  for (auto&& [e, l_p, _] : l_proect_view) {
    if (l_p == project_root_) {
      return entt::handle{*g_reg(), e};
    }
  }

  entt::handle l_handle{*g_reg(), g_reg()->create()};
  l_handle.emplace<project>(project_root_);
  l_handle.emplace<assets>(project_root_.p_name);
  l_handle.emplace<database>();
  return l_handle;
}

entt::handle scan_category_data_t::get_assets_handle() const {
  auto l_prj_handle = get_project_handle();

  for (auto l_c : l_prj_handle.get<assets>().get_child()) {
    if (l_c && l_c.all_of<assets>() && l_c.get<assets>().p_path == file_type_.p_path) return l_c;
  }

  entt::handle l_handle{*g_reg(), g_reg()->create()};

  l_handle.emplace<assets>(file_type_.p_path);
  l_handle.emplace<database>();

  l_prj_handle.get<assets>().add_child(l_handle);
  return l_handle;
}

}  // namespace doodle::details