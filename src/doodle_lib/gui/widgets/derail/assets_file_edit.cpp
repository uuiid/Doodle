//
// Created by td_main on 2023/6/9.
//

#include "assets_file_edit.h"

#include <doodle_core/metadata/assets_file.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <doodle_lib/gui/widgets/derail/user_edit.h>

#include "imgui.h"
#include "imgui_stdlib.h"
namespace doodle::gui::render {

bool assets_file_edit(const entt::handle& in_handle_view) {
  using gui_data = assets_file_edit_t;
  if (!in_handle_view.all_of<assets_file>()) return false;

  auto& l_assets_file = in_handle_view.get<assets_file>();
  auto& l_gui_id      = in_handle_view.get_or_emplace<gui_data>(in_handle_view);
  bool on_change{false};

  if (ImGui::InputText(*l_gui_id.path_id, &l_gui_id.path)) {
    in_handle_view.patch<assets_file>().path_attr(l_gui_id.path);
    on_change = true;
  }

  if (ImGui::InputText(*l_gui_id.name_id, &l_gui_id.name)) {
    in_handle_view.patch<assets_file>().name_attr(l_gui_id.name);
    on_change = true;
  }

  if (ImGui::InputInt(*l_gui_id.version_id, &l_gui_id.version)) {
    in_handle_view.patch<assets_file>().version_attr(l_gui_id.version);
    on_change = true;
  }
  if (auto [l_edit, l_h] = select_all_user(l_gui_id.user, g_reg()); l_edit) {
    in_handle_view.patch<assets_file>().user_attr(l_h);
    on_change = true;
  }

  return on_change;
}

}  // namespace doodle::gui::render