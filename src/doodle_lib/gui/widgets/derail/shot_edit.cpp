//
// Created by td_main on 2023/6/9.
//

#include "shot_edit.h"

#include <doodle_core/metadata/shot.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui::render {

bool shot_edit_t::render(const entt::handle& in_handle_view) {
  bool on_change{false};
  if (in_handle_view.all_of<shot>()) {
    auto& l_shot = in_handle_view.get<shot>();
    if (ImGui::InputInt(*id, &l_shot.p_shot)) {
      in_handle_view.patch<shot>();
      on_change = true;
    }
    if (dear::Combo const l_gui{*ab_id, l_shot.p_shot_ab.c_str()}) {
      static auto shot_enum{magic_enum::enum_names<shot::shot_ab_enum>()};
      for (auto& i : shot_enum) {
        if (imgui::Selectable(i.data(), i == l_shot.p_shot_ab)) {
          l_shot.p_shot_ab = i;
          in_handle_view.patch<shot>();
          on_change = true;
        }
      }
    }
  } else {
    if (ImGui::Button(*add)) {
      in_handle_view.emplace<shot>();
      on_change = true;
    }
  }
  return on_change;
}
}  // namespace doodle::gui::render