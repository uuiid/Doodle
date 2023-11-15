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
  if (!render_id_.any_of<file_association_ref>()) {
    render_id_.emplace<file_association_ref>();
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
entt::handle file_association_edit_t::create_file_association_handle(const entt::handle& in_handle) {
  entt::handle l_r{};
  if (render_id_.any_of<file_association_ref>()) {
    l_r = render_id_.get<file_association_ref>();
  }
  if (in_handle && in_handle.any_of<file_association_ref>()) {
    l_r = in_handle.get<file_association_ref>();
  }
  if (!l_r) {
    l_r = entt::handle{*g_reg(), g_reg()->create()};
    l_r.emplace<database>();
  }
  if (!l_r.any_of<database>()) {
    l_r.emplace<database>();
  }
  return l_r;
}

bool file_association_edit_t::render(const entt::handle& in_handle_view) {
  init(in_handle_view);

  bool on_change{false};

  dear::Text(tool_tip_);
  if (ImGui::InputText(*name_id_, &name_)) {
    create_file_association_handle().get_or_emplace<file_association>().name = name_;
    on_change                                                                = true;
  }

  if (ImGui::Button("关联自身##1")) {
    entt::handle l_file_ref      = create_file_association_handle();
    auto& l_file_association     = l_file_ref.get_or_emplace<file_association>();
    l_file_association.maya_file = render_id_;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    maya_file_ = render_id_.get<doodle::assets_file>().path_attr().generic_string();
    on_change  = true;
  }
  ImGui::SameLine();
  ImGui::InputText(*maya_file_id, &maya_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    entt::handle l_file_ref      = create_file_association_handle(l_h);
    auto& l_file_association     = l_file_ref.get_or_emplace<file_association>();
    l_file_association.maya_file = l_h;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    l_h.emplace_or_replace<file_association_ref>(l_file_ref);
    maya_file_ = l_h.get<doodle::assets_file>().path_attr().generic_string();
    on_change  = true;
  }

  if (ImGui::Button("关联自身##2")) {
    entt::handle l_file_ref          = create_file_association_handle();

    auto& l_file_association         = l_file_ref.get_or_emplace<file_association>();
    l_file_association.maya_rig_file = render_id_;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    maya_rig_file_ = render_id_.get<doodle::assets_file>().path_attr().generic_string();
    on_change      = true;
  }
  ImGui::SameLine();
  ImGui::InputText(*maya_rig_file_id, &maya_rig_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    entt::handle l_file_ref          = create_file_association_handle(l_h);

    auto& l_file_association         = l_file_ref.get_or_emplace<file_association>();
    l_file_association.maya_rig_file = l_h;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    l_h.emplace_or_replace<file_association_ref>(l_file_ref);
    maya_rig_file_ = l_h.get<doodle::assets_file>().path_attr().generic_string();
    on_change      = true;
  }

  if (ImGui::Button("关联自身##3")) {
    entt::handle l_file_ref    = create_file_association_handle();
    auto& l_file_association   = l_file_ref.get_or_emplace<file_association>();
    l_file_association.ue_file = render_id_;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    ue_file_  = render_id_.get<doodle::assets_file>().path_attr().generic_string();
    on_change = true;
  }
  ImGui::SameLine();
  ImGui::InputText(*ue_file_id, &ue_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    entt::handle l_file_ref    = create_file_association_handle(l_h);
    auto& l_file_association   = l_file_ref.get_or_emplace<file_association>();
    l_file_association.ue_file = l_h;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    l_h.emplace_or_replace<file_association_ref>(l_file_ref);
    ue_file_  = l_h.get<doodle::assets_file>().path_attr().generic_string();
    on_change = true;
  }

  if (ImGui::Button("关联自身##4")) {
    entt::handle l_file_ref           = create_file_association_handle();
    auto& l_file_association          = l_file_ref.get_or_emplace<file_association>();
    l_file_association.ue_preset_file = render_id_;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    ue_preset_file_ = render_id_.get<doodle::assets_file>().path_attr().generic_string();
    on_change       = true;
  }
  ImGui::SameLine();
  ImGui::InputText(*ue_preset_file_id, &ue_preset_file_, ImGuiInputTextFlags_ReadOnly);
  if (auto l_h = get_drop_handle()) {
    entt::handle l_file_ref           = create_file_association_handle(l_h);
    auto& l_file_association          = l_file_ref.get_or_emplace<file_association>();
    l_file_association.ue_preset_file = l_h;
    render_id_.emplace_or_replace<file_association_ref>(l_file_ref);
    l_h.emplace_or_replace<file_association_ref>(l_file_ref);
    ue_preset_file_ = l_h.get<doodle::assets_file>().path_attr().generic_string();
    on_change       = true;
  }

  return on_change;
}

}  // namespace doodle::gui::render