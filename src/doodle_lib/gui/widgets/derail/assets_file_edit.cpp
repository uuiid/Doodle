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

void assets_file_edit_t::init(const entt::handle& in_handle) {
  if (in_handle == render_id) return;

  if (!user_edit) user_edit = std::make_shared<select_all_user_t>();

  if (in_handle.any_of<assets_file>()) {
    auto& l_assets_file = in_handle.get<assets_file>();
    path                = l_assets_file.path_attr().generic_string();
    name                = l_assets_file.name_attr();
    version             = l_assets_file.version_attr();
    user_edit->set_current_user(l_assets_file.user_attr());
  };
  render_id = in_handle;
}
bool assets_file_edit_t::render(const entt::handle& in_handle_view) {
  init(in_handle_view);

  bool on_change{false};
  if (!in_handle_view.any_of<assets_file>()) {
    return on_change;
  }
  if (ImGui::InputText(*path_id, &path)) {
    in_handle_view.patch<assets_file>().path_attr(path);
    on_change = true;
  }
  if (ImGui::InputText(*name_id, &name)) {
    in_handle_view.patch<assets_file>().name_attr(name);
    on_change = true;
  }
  if (ImGui::InputInt(*version_id, &version)) {
    in_handle_view.patch<assets_file>().version_attr(version);
    on_change = true;
  }
  if (auto [l_edit, l_h] = user_edit->render(g_reg()); l_edit) {
    in_handle_view.patch<assets_file>().user_attr(l_h);
    on_change = true;
  }

  return on_change;
}
}  // namespace doodle::gui::render