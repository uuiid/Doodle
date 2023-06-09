//
// Created by td_main on 2023/6/9.
//

#include "season_edit.h"

#include <doodle_core/metadata/season.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle::gui::render {

bool season_edit_t::render(const entt::handle& in_handle_view) {
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