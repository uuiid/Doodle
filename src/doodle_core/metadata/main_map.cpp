//
// Created by TD on 2023/11/21.
//

#include "main_map.h"

#include <doodle_core/metadata/assets_file.h>
namespace doodle {

namespace {

FSys::path find_u_pej(const FSys::path& in_path) {
  if (in_path.root_path() == in_path) return {};
  if (!FSys::exists(in_path)) return {};
  if (!FSys::is_directory(in_path)) return find_u_pej(in_path.parent_path());

  for (auto&& l_file : FSys::directory_iterator(in_path)) {
    if (l_file.path().extension() == ".uproject") {
      return l_file.path();
    }
  }
  return find_u_pej(in_path.parent_path());
}

}  // namespace

void ue_main_map::find_ue_project_file(const entt::handle_view<assets_file, ue_main_map>& in_handle_view) {
  auto& l_path = in_handle_view.get<assets_file>().path_attr();
  if (l_path.empty()) return;
  auto l_upej = find_u_pej(l_path);
  if (!l_upej.empty()) in_handle_view.emplace_or_replace<ue_main_map>(l_upej);
}
FSys::path ue_main_map::find_ue_project_file(const FSys::path& in_path) {
  if (in_path.empty()) return {};
  return find_u_pej(in_path);
}

}  // namespace doodle