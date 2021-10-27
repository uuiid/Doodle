//
// Created by TD on 2021/9/16.
//

#include "assets_widget.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/hana/ext/std.hpp>
#include <boost/range/any_range.hpp>
namespace doodle {

assets_widget::assets_widget()
    : p_root(),
      p_meta(),
      p_all_old_selected(),
      p_all_selected(),
      reg(g_reg()) {
  p_factory    = new_object<attr_assets>();
  p_class_name = "资产";
}
void assets_widget::frame_render() {
  dear::TreeNode{"assets_widget"} && [this]() {
    if (auto k_i = reg->try_ctx<project_ref>(); k_i) {
      load_meta(to_entity((*k_i).get()));
    }
  };
  p_all_old_selected = p_all_selected;
}

void assets_widget::set_metadata(const entt::entity& in_ptr) {
  p_root = in_ptr;
}
void assets_widget::load_meta(const entt::entity& in_ptr) {
  static auto base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                         ImGuiTreeNodeFlags_SpanAvailWidth};
  auto l_h    = make_handle(in_ptr);
  auto l_tree = l_h.try_get<tree_relationship>();
  if (!l_tree)
    return;
  auto& l_data = l_h.get<database>();
  if (l_tree && l_data.has_child()) {
    for (const auto& i : l_tree->get_child()) {
      auto flsge = base_flags;
      if (is_select(i))
        flsge |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

      auto l_h1      = make_handle(i);
      auto& l_data1  = l_h1.get<database>();

      auto l_str_com = l_h1.try_get<season, episodes, shot, assets>();
      string l_str{};
      boost::hana::for_each(l_str_com, [&](auto& in_ptr) {
        if (in_ptr && l_str.empty()) {
          l_str = in_ptr->str();
        }
      });

      if (l_data1.has_child() || l_data1.has_file()) {
        bool checked = false;  //使用这个变量标记为只检查一次
        dear::TreeNodeEx{
            l_data1.get_uuid().c_str(),
            flsge,
            l_str.c_str()} &&
            [&l_data1,i, this, &checked]() {
              if (!checked) {
                check_item_clicked(i);
                checked = true;
              }
              //添加文件标志
              if (l_data1.has_file())
                imgui::BulletText("files");
              load_meta(i);
            };
        if (!checked)
          check_item_clicked(l_h1);
        check_item(i);
      } else {
        dear::TreeNodeEx{
            l_data1.get_uuid().c_str(),
            flsge | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
            l_str.c_str()} &&
            [this, i]() {
            };
        check_item_clicked(i);
        check_item(i);
      }
    }
  }
}

bool assets_widget::is_select(const entt::entity& in_ptr) {
  return std::any_of(p_all_old_selected.begin(), p_all_old_selected.end(),
                     [&](const entt::entity& in_) {
    return in_ == in_ptr;
  });
}

void assets_widget::check_item(const entt::entity& in_ptr) {
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

void assets_widget::check_item_clicked(const entt::entity& in_ptr) {
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
void assets_widget::set_select(const entt::entity& in_ptr) {
  p_meta = in_ptr;
  p_all_selected.insert(p_meta);
  select_change(p_meta);
}
}  // namespace doodle
