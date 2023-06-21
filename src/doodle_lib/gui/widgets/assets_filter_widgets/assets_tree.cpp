//
// Created by td_main on 2023/6/16.
//

#include "assets_tree.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/metadata.h"
#include <doodle_core/configure/static_value.h>
#include <doodle_core/metadata/assets.h>

#include "doodle_app/gui/base/ref_base.h"
#include <doodle_app/lib_warp/imgui_warp.h>

#include "entt/entity/fwd.hpp"
#include "imgui.h"
#include "range/v3/algorithm/for_each.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/transform.hpp"

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
  edit_data = false;
  return render_child(tree_.begin());
}

void assets_tree::popen_menu(const tree_type_t::iterator_base &in) {
  ImGui::InputText(*input_data.input, &input_data.node_name);
  ImGui::SameLine();

  if (ImGui::Button(*input_data.node)) {
    auto l_parent = tree_type_t ::parent(in);
    if (auto it = ranges::find_if(
            tree_type_t::begin(l_parent), tree_type_t::end(l_parent),
            [&](const assets_tree_node &in) -> bool {
              //              DOODLE_LOG_INFO("检查节点 {}", in.name);
              return in.name.name == input_data.node_name;
            }
        );
        it == in.end()) {
      DOODLE_LOG_INFO("添加节点 {}", input_data.node_name);
      entt::handle const l_h{*g_reg(), g_reg()->create()};
      l_h.emplace<database>();
      l_h.emplace<assets>(input_data.node_name);
      l_parent->handle.get<assets>().add_child(l_h);

      tree_.insert(in, assets_tree_node{input_data.node_name, l_h});
      //      edit_data = true;
      ImGui::CloseCurrentPopup();
    }
  }
  ImGui::InputText(*rename_data.input, &rename_data.node_name);
  ImGui::SameLine();

  if (ImGui::Button(*rename_data.rename)) {
    auto l_parent = tree_type_t ::parent(in);
    if (auto it = ranges::find_if(
            tree_type_t::begin(l_parent), tree_type_t::end(l_parent),
            [&](const assets_tree_node &in) -> bool {
              //              DOODLE_LOG_INFO("检查节点 {}", in.name);
              return in.name.name == rename_data.node_name;
            }
        );
        it == in.end()) {
      DOODLE_LOG_INFO("重命名节点 {}", rename_data.node_name);
      if (auto l_h = in->handle; l_h && l_h.all_of<assets>()) {
        l_h.get<assets>().set_path(rename_data.node_name);
        in->name = gui_cache_name_id{rename_data.node_name};
        //        edit_data = true;
      }
      ImGui::CloseCurrentPopup();
    }
  }
}

bool assets_tree::render_child(const tree_type_t::iterator &in_node) {
  for (auto it = tree_type_t::begin(in_node); it != tree_type_t::end(in_node); ++it) {
    ImGuiTreeNodeFlags k_f{assets_tree_node_base_flags};
    if (it->has_select) k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
    const auto l_has_child = it.number_of_children() != 0;
    if (!l_has_child) k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    const auto l_root_node = dear::TreeNodeEx{*it->name, k_f};
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      ranges::for_each(tree_, [](assets_tree_node &in_node) { in_node.has_select = false; });
      it->has_select        = true;
      edit_data             = true;
      current_select_handle = it->handle;
      filter_list();
    }
    if (auto l_popen_menu = dear::PopupContextItem{}) {
      popen_menu(it);
    }
    if (auto l_drag_drop = dear::DragDropTarget{}) {
      if (const auto *l_hs = ImGui::AcceptDragDropPayload(doodle_config::drop_handle_list.data()); l_hs) {
        auto *l_list = reinterpret_cast<std::vector<entt::handle> *>(l_hs->Data);
        ranges::for_each(
            *l_list | ranges::views::filter([](entt::handle &in_handle) -> bool {
              return in_handle && in_handle.all_of<assets_file>();
            }),
            [&](entt::handle &in_h) { in_h.patch<assets_file>().assets_attr(it->handle); }
        );
      }
    }
    if (l_has_child && l_root_node) {
      edit_data |= render_child(it);
    }
  }
  return edit_data;
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
  auto l_view = g_reg()->view<assets_file>();
  filter_list_handles =
      l_view | ranges::views::transform([](const decltype(l_view)::entity_type &in_value_type) -> entt::handle {
        return {*g_reg(), in_value_type};
      }) |
      ranges::to_vector;
}
void assets_tree::filter_list() {
  auto l_view         = g_reg()->view<assets_file>().each();
  filter_list_handles = l_view |
                        ranges::views::filter([this](const decltype(l_view)::value_type &in_value_type) -> bool {
                          auto &&[l_e, l_ass] = in_value_type;
                          return l_ass.assets_attr() == current_select_handle;
                        }) |
                        ranges::views::transform([](const decltype(l_view)::value_type &in_value_type) -> entt::handle {
                          auto &&[l_e, l_ass] = in_value_type;
                          return {*g_reg(), l_e};
                        }) |
                        ranges::to_vector;
}
}  // namespace doodle::gui