//
// Created by td_main on 2023/6/16.
//

#include "assets_tree.h"

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/configure/static_value.h>
#include <doodle_core/metadata/assets.h>

#include <doodle_app/lib_warp/imgui_warp.h>

#include "imgui.h"
#include "range/v3/algorithm/for_each.hpp"

namespace doodle::gui {

constexpr const static ImGuiTreeNodeFlags assets_tree_node_base_flags{
    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth};

bool assets_tree::assets_tree_node::operator<(const doodle::gui::assets_tree::assets_tree_node &rhs) const {
  return name < rhs.name;
}

void assets_tree::build_tree(const entt::handle &in_handle_view, const tree_type_t::iterator &in_parent) {
  auto &l_ass = in_handle_view.get<assets>();
  auto l_it   = tree_.append_child(in_parent, assets_tree_node{l_ass.get_path(), in_handle_view});

  for (auto &&l_c : l_ass.get_child()) {
    build_tree(l_c, l_it);
  }
}

bool assets_tree::render() {
  render_child(tree_.begin());
  return true;
}
void assets_tree::popen_menu(const tree_type_t::iterator_base &in) {
  ImGui::InputText(*input_add_node, &node_name);
  ImGui::SameLine();

  if (ImGui::Button("添加")) {
    auto l_parent = tree_type_t ::parent(in);
    if (auto it = ranges::find_if(
            tree_type_t::begin(l_parent), tree_type_t::end(l_parent),
            [&](const assets_tree_node &in) -> bool {
              //              DOODLE_LOG_INFO("检查节点 {}", in.name);
              return in.name == node_name;
            }
        );
        it == in.end()) {
      DOODLE_LOG_INFO("添加节点 {}", node_name);
      tree_.insert(in, assets_tree_node{node_name, {}});
      //      auto k_path = in_node.data.data / p_popen.data;
      //      in_node.child.emplace_back(std::make_shared<tree_node_type>(gui_cache_path{p_popen.data, k_path}));
      ImGui::CloseCurrentPopup();
    }
  }
}

bool assets_tree::render_child(const tree_type_t::iterator &in_node) {
  for (auto it = tree_type_t::begin(in_node); it != tree_type_t::end(in_node); ++it) {
    ImGuiTreeNodeFlags k_f{assets_tree_node_base_flags};
    if (it->has_select) k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
    const auto l_has_child = it.number_of_children() != 0;
    if (!l_has_child) k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf;

    const auto l_root_node = dear::TreeNodeEx{it->name.c_str(), k_f};
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      ranges::for_each(tree_, [](assets_tree_node &in_node) { in_node.has_select = false; });
      it->has_select = true;
    }
    if (auto l_popen_menu = dear::PopupContextItem{}) {
      popen_menu(it);
    }
    if (auto l_drag_drop = dear::DragDropTarget{}) {
      if (const auto *l_hs = ImGui::AcceptDragDropPayload(doodle_config::drop_handle_list.data()); l_hs) {
        auto *l_list = reinterpret_cast<std::vector<entt::handle> *>(l_hs->Data);
        for (auto &&l_h : *l_list) {
          //          l_h.emplace_or_replace<assets>(i->data.data);
        }
        // @todo 这里要写拖拽
      }
    }
    if (l_has_child && l_root_node) {
      render_child(it);
    }
  }
  return false;
}
void assets_tree::init_tree() {
  tree_.clear();
  tree_           = tree_type_t{assets_tree_node{"root", entt::handle{*g_reg(), entt::null}}};
  auto l_ass_view = g_reg()->view<assets>();
  for (auto &&[e, ass] : l_ass_view.each()) {
    if (!ass.get_parent()) {
      auto l_h = entt::handle{*g_reg(), e};
      //      auto l_it = tree_.insert(tree_.begin(), assets_tree_node{ass.p_path, l_h});
      build_tree(l_h, tree_.begin());
    }
  }
}
}  // namespace doodle::gui