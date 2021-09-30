//
// Created by TD on 2021/9/16.
//

#include "assets_widget.h"

#include <doodle_lib/Gui/factory/attribute_factory_interface.h>
#include <doodle_lib/Metadata/metadata_cpp.h>
#include <doodle_lib/libWarp/imgui_warp.h>

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/range/any_range.hpp>

namespace doodle {

assets_widget::assets_widget()
    : p_root(),
      p_meta(),
      p_all_old_selected(),
      p_all_selected() {
  p_factory    = new_object<attr_assets>();
  p_class_name = "资产";
}
void assets_widget::frame_render() {
  dear::TreeNode{"assets_widget"} && [this]() {
    if (p_root && p_root->has_child()) {
      load_meta(p_root);
    }
  };
  p_all_old_selected = p_all_selected;
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

    for (const auto& i : in_ptr->get_child()) {
      auto flsge = base_flags;
      if (is_select(i))
        flsge |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

      if (i->has_child() || i->has_file()) {
        bool checked = false;  //使用这个变量标记为只检查一次
        dear::TreeNodeEx{
            i->get_uuid().c_str(),
            flsge,
            i->show_str().c_str()} &&
            [i, this, &checked]() {
              if (!checked) {
                check_item_clicked(i);
                checked = true;
              }
              //添加文件标志
              if (i->has_file())
                imgui::BulletText("files");
              load_meta(i);
            };
        if (!checked)
          check_item_clicked(i);
        check_item(i);
      } else {
        dear::TreeNodeEx{
            i->get_uuid().c_str(),
            flsge | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
            i->show_str().c_str()} &&
            [this, i]() {
            };
        check_item_clicked(i);
        check_item(i);
      }
    }
  }
}

bool assets_widget::is_select(const metadata_ptr& in_ptr) {
  return std::any_of(p_all_old_selected.begin(), p_all_old_selected.end(), [&](const metadata_ptr& in_) {
    return in_ == in_ptr;
  });
}

void assets_widget::check_item(const metadata_ptr& in_ptr) {
  // if (imgui::IsItemHovered()) {
  //   DOODLE_LOG_DEBUG("ok");
  // }
  if (imgui::GetIO().KeyShift && imgui::IsItemHovered()) {
    p_all_selected.insert(in_ptr);
  }
  // if (imgui::GetIO().KeyCtrl && imgui::IsItemHovered()) {
  //   p_all_selected.erase(in_ptr);
  // }
}

void assets_widget::check_item_clicked(const metadata_ptr& in_ptr) {
  if (imgui::IsItemClicked()) {
    if (!imgui::GetIO().KeyCtrl) {
      p_all_selected.clear();
      p_all_old_selected.clear();
    } else {
      p_all_selected.erase(in_ptr);
      return;
    }
    set_select(in_ptr);
  }
}
void assets_widget::set_select(const metadata_ptr& in_ptr) {
  p_meta = in_ptr;
  p_all_selected.insert(p_meta);
  p_meta->attribute_widget(p_factory);
  select_change(p_meta);
}
}  // namespace doodle
