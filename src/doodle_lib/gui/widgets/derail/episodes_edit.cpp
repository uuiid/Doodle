//
// Created by td_main on 2023/6/9.
//

#include "episodes_edit.h"

#include "doodle_core/metadata/episodes.h"

#include <doodle_app/gui/base/ref_base.h>

#include <imgui.h>
namespace doodle::gui::render {
bool episodes_edit(const entt::handle& in_handle_view) {
  using gui_data = gui_cache_name_id_temp<episodes_edit_t>;

  if (in_handle_view.all_of<episodes>()) {
    auto& l_eps    = in_handle_view.get<episodes>();
    auto& l_gui_id = in_handle_view.get_or_emplace<gui_data>("集数:"s);
    if (ImGui::InputInt(*l_gui_id, &l_eps.p_episodes)) {
      in_handle_view.patch<episodes>();
      return true;
    }
  }

  return false;
}
}  // namespace doodle::gui::render