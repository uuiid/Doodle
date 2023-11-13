//
// Created by td_main on 2023/11/13.
//
#include "file_association_edit.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/file_association.h>

#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle::gui::render {
void file_association_edit_t::init(const entt::handle& in_handle) {
  if (in_handle == render_id_) return;

  if (in_handle.any_of<file_association_ref>() && in_handle.get<file_association_ref>()) {
    auto& l_path = in_handle.get<file_association_ref>().get<file_association>();
    name_        = l_path.name;

    if (auto l_h = l_path.maya_file; l_h) maya_file_ = l_h.get<doodle::assets_file>().path_attr().generic_string();
    if (auto l_h = l_path.maya_rig_file; l_h)
      maya_rig_file_ = l_h.get<doodle::assets_file>().path_attr().generic_string();
    if (auto l_h = l_path.ue_file; l_h) ue_file_ = l_h.get<doodle::assets_file>().path_attr().generic_string();
    if (auto l_h = l_path.ue_preset_file; l_h)
      ue_preset_file_ = l_h.get<doodle::assets_file>().path_attr().generic_string();
  } else {
    name_ = "未建立任何联系";
    maya_file_.clear();
    maya_rig_file_.clear();
    ue_file_.clear();
    ue_preset_file_.clear();
  }
  render_id_ = in_handle;
}
void file_association_edit_t::create_file_association() {
  if (!render_id_.any_of<file_association>()) {
    render_id_.emplace<file_association>();
    if (!render_id_.any_of<database>()) render_id_.emplace<database>();
  }
}

entt::handle file_association_edit_t::get_drop_handle() {
  if (auto l_drag = dear::DragDropTarget{}) {
    if (const auto* l_data = ImGui::AcceptDragDropPayload(doodle_config::drop_handle_list.data());
        l_data && l_data->IsDelivery()) {
      auto* l_list = static_cast<std::vector<entt::handle>*>(l_data->Data);
      if (l_list->size() == 2 && l_list->front() == render_id_) {
        return l_list->back();
      }
    }
  }
  return {};
}

bool file_association_edit_t::render(const entt::handle& in_handle_view) {
  init(in_handle_view);

  bool on_change{false};

  dear::Text(name_);
  dear::Text(tool_tip_);

  ImGui::InputText(*maya_file_id, &maya_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    create_file_association();
    auto& l_path                                          = l_h.get<assets_file>();
    maya_file_                                            = l_path.path_attr().generic_string();
    in_handle_view.patch<file_association>().maya_file    = l_h;
    on_change                                             = true;
  }

  ImGui::InputText(*maya_rig_file_id, &maya_rig_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    create_file_association();
    auto& l_path                                           = l_h.get<assets_file>();
    maya_rig_file_                                         = l_path.path_attr().generic_string();
    in_handle_view.patch<file_association>().maya_rig_file = l_h;
    on_change                                              = true;
  }

  ImGui::InputText(*ue_file_id, &ue_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    create_file_association();
    auto& l_path                                     = l_h.get<assets_file>();
    ue_file_                                         = l_path.path_attr().generic_string();
    in_handle_view.patch<file_association>().ue_file = l_h;
    on_change                                        = true;
  }

  ImGui::InputText(*ue_preset_file_id, &ue_preset_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    create_file_association();
    auto& l_path                                            = l_h.get<assets_file>();
    ue_preset_file_                                         = l_path.path_attr().generic_string();
    in_handle_view.patch<file_association>().ue_preset_file = l_h;
    on_change                                               = true;
  }

  return on_change;
}

}  // namespace doodle::gui::render