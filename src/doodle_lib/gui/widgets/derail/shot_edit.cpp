//
// Created by td_main on 2023/6/9.
//

#include "shot_edit.h"

#include <doodle_core/metadata/shot.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui::render {

bool shot_edit(const entt::handle& in_handle_view) {
  using gui_data    = gui_cache_name_id_temp<shot_edit_t>;
  using gui_ab_data = gui_cache_name_id_temp<shot_ab_edit_t>;
  bool is_change{false};
  if (!in_handle_view.all_of<shot>()) return is_change;

  auto& l_shot   = in_handle_view.get<shot>();
  auto& l_gui_id = in_handle_view.get_or_emplace<gui_data>("镜头:"s);
  if (ImGui::InputInt(*l_gui_id, &l_shot.p_shot)) {
    in_handle_view.patch<shot>();
    is_change = true;
  }
  auto& l_shot_ab = in_handle_view.get_or_emplace<gui_ab_data>("ab镜头:"s);

  if (dear::Combo const l_gui{*l_shot_ab, l_shot.p_shot_ab.c_str()}) {
    static auto shot_enum{magic_enum::enum_names<shot::shot_ab_enum>()};
    for (auto& i : shot_enum) {
      if (imgui::Selectable(i.data(), i == l_shot.p_shot_ab)) {
        l_shot.p_shot_ab = i;
        in_handle_view.patch<shot>();
        is_change = true;
      }
    }
  }

  return is_change;
}
}  // namespace doodle::gui::render