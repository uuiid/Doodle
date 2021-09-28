//
// Created by TD on 2021/9/16.
//

#include "assets_widget.h"

#include <DoodleLib/Gui/factory/attribute_factory_interface.h>
#include <DoodleLib/Metadata/metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/range/any_range.hpp>

namespace doodle {

assets_widget::assets_widget()
    : p_root(),
      p_meta() {
  p_factory    = new_object<attr_assets>();
  p_class_name = "资产";
}
void assets_widget::frame_render() {
  dear::TreeNode{"assets_widget"} && [this]() {
    if (p_root && p_root->has_child()) {
      load_meta(p_root);
    }
  };
}

void assets_widget::set_metadata(const metadata_ptr& in_ptr) {
  p_root = in_ptr;
}
void assets_widget::load_meta(const metadata_ptr& in_ptr) {
  static auto base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                         ImGuiTreeNodeFlags_SpanAvailWidth};
  if (in_ptr && in_ptr->has_child()) {
    in_ptr->select_indb();

    for (const auto& i : in_ptr->child_item) {
      auto flsge = base_flags;
      if (p_meta == i)
        flsge |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

      if (i->has_child() || i->has_file()) {
        dear::TreeNodeEx{
            i->get_uuid().c_str(),
            flsge,
            i->show_str().c_str()} &&
            [i, this]() {
              // imgui::SameLine();
              if (imgui::IsItemClicked())
                set_select(i);
              if (i->has_file())
                imgui::BulletText("files");
              load_meta(i);
            };
      } else {
        dear::TreeNodeEx{
            i->get_uuid().c_str(),
            flsge | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
            i->show_str().c_str()} &&
            [this, i]() {
              if (imgui::IsItemClicked()) {
                set_select(i);
              }
            };
      }
    }
  }
}
void assets_widget::set_select(const metadata_ptr& in_ptr) {
  p_meta = in_ptr;
  p_meta->attribute_widget(p_factory);
  select_change(p_meta);
}
}  // namespace doodle
