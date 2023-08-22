//
// Created by td_main on 2023/8/22.
//

#include "render_monitor.h"

#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle {
namespace gui {
void render_monitor::init() {}
bool render_monitor::render() {
  if (auto l_ = dear::CollapsingHeader{*p_i->component_collapsing_header_id_}) {
    if (auto l_table = dear::Table{*p_i->component_table_id_, 3}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();
      for (auto& l_computer : p_i->computers_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%llu", l_computer.id_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.name_);
        ImGui::TableNextColumn();
        dear::Text(l_computer.state_);
      }
    }
  }

  if (auto l_ = dear::CollapsingHeader{*p_i->render_task_collapsing_header_id_}) {
    if (auto l_table = dear::Table{*p_i->render_task_table_id_, 3}) {
      ImGui::TableSetupScrollFreeze(0, 1);  // Make top row always visible
      ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 100.0f);
      ImGui::TableSetupColumn("名称");
      ImGui::TableSetupColumn("状态");

      ImGui::TableHeadersRow();

      for (auto& l_render_task : p_i->render_tasks_) {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::Text("%llu", l_render_task.id_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.name_);
        ImGui::TableNextColumn();
        dear::Text(l_render_task.state_);
      }
    }
  }

  return p_i->open_;
}
}  // namespace gui
}  // namespace doodle