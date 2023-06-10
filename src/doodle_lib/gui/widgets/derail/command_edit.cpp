//
// Created by td_main on 2023/6/9.
//

#include "command_edit.h"

#include <doodle_core/metadata/comment.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle::gui::render {

bool command_edit_t::render(const entt::handle& in_handle_view) {
  bool on_change = false;
  if (in_handle_view.all_of<comment>()) {
    auto& l_comm = in_handle_view.get<comment>();

    if (ImGui::InputText(*id, &l_comm.p_comment)) {
      in_handle_view.patch<comment>();
      on_change = true;
    }
  }
  return on_change;
}
}  // namespace doodle::gui::render