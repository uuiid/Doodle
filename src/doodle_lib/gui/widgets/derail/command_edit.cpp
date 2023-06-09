//
// Created by td_main on 2023/6/9.
//

#include "command_edit.h"

#include <doodle_core/metadata/comment.h>

#include <doodle_app/gui/base/ref_base.h>

#include "imgui_stdlib.h"
namespace doodle::gui::render {

bool command_edit(const entt::handle& in_handle_view) {
  using gui_data = gui_cache_name_id_temp<command_edit_t>;
  if (in_handle_view.all_of<comment>()) {
    auto& l_comm   = in_handle_view.get<comment>();
    auto& l_gui_id = in_handle_view.get_or_emplace<gui_data>("备注:"s);

    if (ImGui::InputText(*l_gui_id, &l_comm.p_comment)) {
      in_handle_view.patch<comment>();
      return true;
    }
  }
  return false;
}
}  // namespace doodle::gui::render