//
// Created by TD on 2023/11/21.
//

#include "ue_main_map_edit.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/platform/win/windows_alias.h>

#include "imgui.h"
namespace doodle::gui::render {

void ue_main_map_edit::test_is_ue_dir(const FSys::path &in_path) {
  if (FSys::is_windows_remote_path(in_path)) return;
  std::error_code l_ec{};
  if (!FSys::exists(in_path, l_ec) || l_ec) return;
  if (is_ue_dir_ || in_path.root_path() == in_path) return;
  if (!FSys::is_directory(in_path)) return test_is_ue_dir(in_path.parent_path());

  for (auto &&l_file : FSys::directory_iterator(in_path)) {
    if (l_file.path().extension() == ".uproject") {
      is_ue_dir_      = true;
      u_project_path_ = l_file.path();
      return;
    }
  }
  return test_is_ue_dir(in_path.parent_path());
}

void ue_main_map_edit::init(const entt::handle &in_handle) {
  if (render_id_ == in_handle) return;

  if (in_handle.all_of<ue_main_map>()) {
    u_project_str_ = in_handle.get<ue_main_map>().map_path_.generic_string();
    render_id_     = in_handle;
    return;
  }
  is_ue_dir_ = false;
  if (in_handle.all_of<assets_file>()) {
    test_is_ue_dir(in_handle.get<assets_file>().path_attr());
  }
  if (is_ue_dir_) {
    in_handle.emplace_or_replace<ue_main_map>(u_project_path_);
  }

  render_id_ = in_handle;
}

bool ue_main_map_edit::render(const entt::handle &in_handle_view) {
  init(in_handle_view);
  bool on_change = false;

  dear::Text(u_project_str_);

  return on_change;
}

}  // namespace doodle::gui::render