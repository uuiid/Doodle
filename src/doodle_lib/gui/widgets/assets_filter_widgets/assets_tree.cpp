//
// Created by td_main on 2023/6/16.
//

#include "assets_tree.h"

#include <doodle_core/configure/static_value.h>

#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui {

constexpr const static ImGuiTreeNodeFlags assets_tree_node_base_flags{
    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth};

bool assets_tree::assets_tree_node::operator<(const doodle::gui::assets_tree::assets_tree_node &rhs) const {
  return name < rhs.name;
}

bool assets_tree::render() {
  for (auto it = tree_.begin_fixed(tree_.begin(), 0); it != tree_.end_fixed(tree_.begin(), 0); ++it) {
    ImGuiTreeNodeFlags k_f{assets_tree_node_base_flags};
    if (it->has_select) k_f |= ImGuiTreeNodeFlags_Selected;
    if (it.number_of_children() != 0) k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf;

    if (const auto l_root_node = dear::TreeNodeEx{it->name.c_str(), assets_tree_node_base_flags}) {
      if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
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
      render_child(it);
    }
  }
  return false;
}
void assets_tree::popen_menu(const tree<assets_tree_node>::iterator_base &in) {}
bool assets_tree::render_child(const tree<assets_tree_node>::iterator &in) { return false; }
}  // namespace doodle::gui