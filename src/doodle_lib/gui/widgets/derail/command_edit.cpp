//
// Created by td_main on 2023/6/9.
//

#include "command_edit.h"

#include <doodle_core/metadata/comment.h>

#include <doodle_app/gui/base/ref_base.h>

#include "imgui_stdlib.h"
namespace doodle::gui::render {

bool command_edit_t::render(const entt::handle& in_handle_view) {
  if (in_handle_view.all_of<comment>()) {
    auto& l_comm = in_handle_view.get<comment>();

    if (ImGui::InputText(*id, &l_comm.p_comment)) {
      in_handle_view.patch<comment>();
      return true;
    }
  }
}
}  // namespace doodle::gui::render