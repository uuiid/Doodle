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
    load_meta(p_root);
  };
  p_all_old_selected = p_all_selected;
}

void assets_widget::set_metadata(const entt::entity& in_ptr) {
  p_root = make_handle(in_ptr);
}
void assets_widget::load_meta(const entt::handle& in_ptr) {
  static auto base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                         ImGuiTreeNodeFlags_SpanAvailWidth};
  if (!in_ptr.all_of<tree_relationship, database>())
    return;

  auto& l_tree = in_ptr.get<tree_relationship>();

  auto& l_data = in_ptr.get<database>();
  if (l_data.has_child()) {
    /// 加载数据
    if (!in_ptr.all_of<is_load>())
      in_ptr.emplace<need_load>();
    for (const auto& i : l_tree.get_child()) {
      auto k_ch  = make_handle(i);
      auto flsge = base_flags;
      if (is_select(k_ch))
        flsge |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

      auto& l_data1  = k_ch.get<database>();
      auto l_str_com = k_ch.try_get<season, episodes, shot, assets>();
      string l_str{k_ch.get_or_emplace<to_str>()};

      if (l_data1.has_child() || l_data1.has_file()) {
        bool checked = false;  //使用这个变量标记为只检查一次
        dear::TreeNodeEx{
            l_data1.get_uuid().c_str(),
            flsge,
            l_str.c_str()} &&
            [&l_data1, i, this, k_ch, &checked]() {
              if (!checked) {
                check_item_clicked(k_ch);
                checked = true;
              }
              //添加文件标志
              if (l_data1.has_file())
                imgui::BulletText("files");
              load_meta(k_ch);
            };
        if (!checked)
          check_item_clicked(k_ch);
        check_item(k_ch);
      } else {
        dear::TreeNodeEx{
            l_data1.get_uuid().c_str(),
            flsge | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
            l_str.c_str()} &&
            [this, k_ch]() {
            };
        check_item_clicked(k_ch);
        check_item(k_ch);
      }
    }
  }
}

bool assets_widget::is_select(const entt::handle& in_ptr) {
  return std::any_of(p_all_old_selected.begin(), p_all_old_selected.end(),
                     [&](const entt::handle& in_) {
                       return in_ == in_ptr;
                     });
}

void assets_widget::check_item(const entt::handle& in_ptr) {
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

void assets_widget::check_item_clicked(const entt::handle& in_ptr) {
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
void assets_widget::set_select(const entt::handle& in_ptr) {
  p_meta = in_ptr;
  p_all_selected.insert(p_meta);
  select_change(p_meta);
}
}  // namespace doodle
