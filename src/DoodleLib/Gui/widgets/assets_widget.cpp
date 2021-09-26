//
// Created by TD on 2021/9/16.
//

#include "assets_widget.h"

#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {

assets_widget::assets_widget()
    : p_root(),
      p_meta() {
  p_factory    = new_object<attr_assets>();
  p_class_name = "资产";
}
void assets_widget::frame_render() {
  dear::TreeNode{"assets_widget"} && [this]() {
    if (p_root && p_root->hasChild()) {
      load_meta(p_root);
    }
  };
}

void assets_widget::set_metadata(const MetadataPtr& in_ptr) {
  p_root = in_ptr;
}
void assets_widget::load_meta(const MetadataPtr& in_ptr) {
  static auto base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                         ImGuiTreeNodeFlags_SpanAvailWidth};
  if (in_ptr && in_ptr->hasChild()) {
    in_ptr->select_indb();
    for (const auto& i : in_ptr->child_item) {
      auto flsge = base_flags;
      if (p_meta == i)
        flsge |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      if (i->hasChild()) {
        //,
        //   p_meta == i ? base_flags | ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected : base_flags
        //
        dear::TreeNodeEx{
            i->getUUID().c_str(),
            flsge,
            i->showStr().c_str()} &&
            [i, this]() {
              if (imgui::IsItemClicked())
                set_select(i);
              load_meta(i);
            };
      } else {
        dear::TreeNodeEx{
            i->getUUID().c_str(),
            flsge | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
            i->showStr().c_str()} &&
            [this, i]() {
              if (imgui::IsItemClicked()) {
                set_select(i);
              }
            };
      }
    }
  }
}
void assets_widget::set_select(const MetadataPtr& in_ptr) {
  p_meta = in_ptr;
  p_meta->create_menu(p_factory);
  select_change(p_meta);
}
}  // namespace doodle
