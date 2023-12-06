//
// Created by TD on 2023/11/21.
//

#include "ue_main_map_edit.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/platform/win/windows_alias.h>

#include "imgui.h"
namespace doodle::gui::render {

void ue_main_map_edit::init(const entt::handle &in_handle) {
  if (render_id_ == in_handle) return;

  if (in_handle.all_of<ue_main_map>()) {
    u_project_str_ = in_handle.get<ue_main_map>().map_path_.generic_string();
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