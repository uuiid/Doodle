//
// Created by td_main on 2023/6/9.
//

#include "importance_edit.h"

#include <doodle_core/metadata/importance.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <imgui.h>
namespace doodle::gui::render {

bool importance_edit_t::render(const entt::handle& in_handle_view) {
  bool on_change{false};
  if (in_handle_view.all_of<importance>()) {
    auto& l_imp = in_handle_view.get<importance>();
    if (ImGui::InputText(*id, &l_imp.cutoff_p)) {
      in_handle_view.patch<importance>();
      on_change = true;
    }
  } else {
    if (ImGui::Button(*add)) {
      in_handle_view.emplace<importance>();
      on_change = true;
    }
  }
  return on_change;
}
}  // namespace doodle::gui::render