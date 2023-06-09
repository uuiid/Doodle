//
// Created by td_main on 2023/6/9.
//

#include "season_render.h"

#include <doodle_core/metadata/season.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle::gui::render {

bool season_render(const entt::handle& in_handle_view) {
  using gui_data = gui_cache_name_id_temp<season_render_t>;

  if (in_handle_view.all_of<season>()) {
    auto& l_season = in_handle_view.get<season>();
    auto& l_gui_id = in_handle_view.get_or_emplace<gui_data>("季数:"s);
    if (ImGui::InputInt(*l_gui_id, &l_season.p_int)) {
      in_handle_view.patch<season>();
      return true;
    }
  }

  return false;
}

bool season_render_t::render(const entt::handle& in_handle_view) {
  if (in_handle_view.all_of<season>()) {
    auto& l_season = in_handle_view.get<season>();
    if (ImGui::InputInt(*id, &l_season.p_int)) {
      in_handle_view.patch<season>();
      return true;
    }
  } else {
    if (ImGui::Button(*add)) {
      in_handle_view.emplace<season>();
      return true;
    }
  }
}
}  // namespace doodle::gui::render