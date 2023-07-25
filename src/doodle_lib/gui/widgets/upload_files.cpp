//
// Created by td_main on 2023/7/12.
//

#include "upload_files.h"

#include "doodle_core/configure/static_value.h"

#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui {
const std::string& upload_files::title() const { return title_; }
bool upload_files::render() {
  constexpr static auto g_text{"拖拽添加文件"};
  if (ImGui::InputText(*ue_file_, &ue_file_path_)) {
  }
  if (auto l_drag = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
        l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      if (!l_list->empty()) {
        ue_file_path_ = l_list->front().generic_string();
      }
    }
  }
  if (auto l_tip = dear::ItemTooltip{}) {
    ImGui::Text(g_text);
  }

  if (ImGui::InputText(*maya_file_, &maya_file_path_)) {
  }
  if (auto l_drag = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
        l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      if (!l_list->empty()) {
        maya_file_path_ = l_list->front().generic_string();
      }
    }
  }

  if (ImGui::InputText(*rig_file_, &rig_file_path_)) {
  }
  if (auto l_drag = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
        l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      if (!l_list->empty()) {
        rig_file_path_ = l_list->front().generic_string();
      }
    }
  }

  if (ImGui::InputText(*ue_file_preset_, &ue_file_preset_path_)) {
  }
  if (auto l_drag = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_imgui_id.data());
        l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<FSys::path>*>(l_data->Data);
      if (!l_list->empty()) {
        ue_file_preset_path_ = l_list->front().generic_string();
      }
    }
  }

  return show_;
}
}  // namespace doodle::gui