//
// Created by td_main on 2023/6/16.
//

#include "assets_tree.h"

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/configure/static_value.h>
#include <doodle_core/metadata/assets.h>

#include <doodle_app/lib_warp/imgui_warp.h>

namespace doodle::gui {

constexpr const static ImGuiTreeNodeFlags assets_tree_node_base_flags{
    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth};

bool assets_tree::assets_tree_node::operator<(const doodle::gui::assets_tree::assets_tree_node &rhs) const {
  return name < rhs.name;
}

void assets_tree::build_tree(const entt::handle &in_handle_view, const tree_type_t::iterator &in_parent) {
  auto &l_ass = in_handle_view.get<assets>();
  auto l_it   = tree_.append_child(in_parent, assets_tree_node{l_ass.get_path(), false, in_handle_view});

  for (auto &&l_c : l_ass.get_child()) {
    build_tree(l_c, l_it);
  }
}

bool assets_tree::render() {
  for (auto it = tree_type_t::begin(tree_.begin()); it != tree_type_t::end(tree_.begin()); ++it) {
    render_child(it);
  }
  return true;
}
void assets_tree::popen_menu(const tree_type_t::iterator_base &in) {}
bool assets_tree::render_child(const tree_type_t::iterator &in_node) {
  for (auto it = tree_type_t::begin(in_node); it != tree_type_t::end(in_node); ++it) {
    ImGuiTreeNodeFlags k_f{assets_tree_node_base_flags};
    if (it->has_select) k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
    if (it.number_of_children() != 0) k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf;

    if (const auto l_root_node = dear::TreeNodeEx{it->name.c_str(), k_f}) {
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
void assets_tree::init_tree() {
  tree_.clear();
  auto l_ass_view = g_reg()->view<assets>();
  for (auto &e : l_ass_view) {
    build_tree({*g_reg(), e}, tree_.begin());
  }
}
}  // namespace doodle::gui