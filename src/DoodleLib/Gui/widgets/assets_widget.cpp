//
// Created by TD on 2021/9/16.
//

#include "assets_widget.h"

#include <DoodleLib/Metadata/Metadata_cpp.h>
#include <DoodleLib/libWarp/imgui_warp.h>
namespace doodle {

assets_widget::assets_widget()
    : p_root(),
      p_meta() {
        
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
  if (in_ptr && in_ptr->hasChild()) {
    in_ptr->select_indb();
    for (const auto& i : in_ptr->child_item) {
      if (i->hasChild()) {
        dear::TreeNode{i->str().c_str()} && [i, this]() {
          if (imgui::IsItemClicked())
            set_select(i);
          load_meta(i);
        };
      } else {
        dear::TreeNodeEx{
            i->str().c_str(),
            ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen} &&
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
  select_change(p_meta);
}
}  // namespace doodle
