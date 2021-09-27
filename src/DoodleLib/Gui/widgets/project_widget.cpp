//
// Created by TD on 2021/9/16.
//

#include "project_widget.h"

#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/metadata_cpp.h>
#include <DoodleLib/core/DoodleLib.h>
#include <DoodleLib/libWarp/imgui_warp.h>

namespace doodle {

project_widget::project_widget()
    : p_current_select() {
  p_factory    = new_object<attr_project>();
  p_class_name = "项目";
}
void project_widget::frame_render() {
  dear::Table{"project", 3} && [this]() {
    imgui::TableSetupColumn("名称");
    imgui::TableSetupColumn("路径");
    imgui::TableSetupColumn("字母名称");
    imgui::TableHeadersRow();

    const auto& k_prj_list = DoodleLib::Get().p_project_vector;

    for (const auto& p : k_prj_list) {
      imgui::TableNextRow();
      imgui::TableNextColumn();
      if (dear::Selectable(p->showStr(),
                           p == p_current_select,
                           ImGuiSelectableFlags_SpanAllColumns)) {
        p_current_select = p;
        p_factory->show_attribute(p_current_select);
        select_change(p_current_select);
      }
      imgui::TableNextColumn();
      dear::Text(p->getPath().generic_string());
      imgui::TableNextColumn();
      dear::Text(p->str());
    }
  };
}

}  // namespace doodle
