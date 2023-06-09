//
// Created by td_main on 2023/6/9.
//

#include "episodes_edit.h"

#include "doodle_core/metadata/episodes.h"

#include <doodle_app/gui/base/ref_base.h>

#include <imgui.h>
namespace doodle::gui::render {

bool episodes_edit_t::render(const entt::handle& in_handle_view) {
  if (in_handle_view.all_of<episodes>()) {
    auto& l_eps = in_handle_view.get<episodes>();
    if (ImGui::InputInt(*id, &l_eps.p_episodes)) {
      in_handle_view.patch<episodes>();
      return true;
    }
  } else {
    if (ImGui::Button(*add)) {
      in_handle_view.emplace<episodes>();
      return true;
    }
  }
  return false;
}
}  // namespace doodle::gui::render