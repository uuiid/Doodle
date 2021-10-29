//
// Created by TD on 2021/9/16.
//

#include "project_widget.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>

namespace doodle {

project_widget::project_widget()
    : p_current_select() {
  p_class_name = "项目";
  auto k_com = new_object<command_base_list>();
  k_com->get();
}
void project_widget::frame_render() {
  dear::Table{"project", 3} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("路径");
    imgui::TableSetupColumn("字母名称");
    imgui::TableHeadersRow();

    const auto& k_prj_list = doodle_lib::Get().p_project_vector;

    for (const auto& p : k_prj_list) {
      auto k_h = make_handle(p);
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(k_h.get<project>().show_str(),
                           p == p_current_select,
                           ImGuiSelectableFlags_SpanAllColumns)) {
        p_current_select = p;
        select_change(p_current_select);
        command_ k_com{comm_project_add{}};
        k_com->render();
      }
      imgui::TableNextColumn();
      dear::Text(k_h.get<project>().get_path().generic_string());
      imgui::TableNextColumn();
      dear::Text(k_h.get<project>().str());
    }
  };
}

}  // namespace doodle
