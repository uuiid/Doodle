//
// Created by td_main on 2023/6/16.
//

#include "assets_tree.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/metadata.h"
#include <doodle_core/configure/static_value.h>
#include <doodle_core/metadata/assets.h>

#include "doodle_app/gui/base/ref_base.h"
#include <doodle_app/lib_warp/imgui_warp.h>

#include "boost/asio/post.hpp"
#include <boost/asio.hpp>

#include "entt/entity/fwd.hpp"
#include "fmt/compile.h"
#include "imgui.h"
#include "range/v3/algorithm/for_each.hpp"
#include "range/v3/range/conversion.hpp"
#include "range/v3/view/filter.hpp"
#include "range/v3/view/transform.hpp"

namespace doodle::gui {

constexpr const static ImGuiTreeNodeFlags assets_tree_node_base_flags{
    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth
};

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

void assets_tree::delete_node_(const tree_type_t::iterator_base &in_node) {
  for (auto it = tree_type_t::begin(in_node); it != tree_type_t::end(in_node); ++it) {
    it->handle.destroy();
    if (it.number_of_children() != 0) {
      delete_node_(it);
    }
  }
}

bool assets_tree::render() {
  edit_data = false;
  const ImGuiTreeNodeFlags k_f{assets_tree_node_base_flags};
  if (auto l_root = dear::TreeNodeEx{"root", k_f | ImGuiTreeNodeFlags_DefaultOpen}) {
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      ranges::for_each(tree_, [](assets_tree_node &in_node) { in_node.has_select = false; });
      edit_data             = true;
      current_select_handle = {};
      filter_list();
    }
    if (auto l_popen_menu = dear::PopupContextItem{}) {
      ImGui::InputText(*input_data.input, &input_data.node_name);
      ImGui::SameLine();

      if (ImGui::Button(*input_data.node_child)) {
        auto l_root_it = tree_.begin();
        if (auto it = ranges::find_if(
                tree_type_t::begin(l_root_it), tree_type_t::end(l_root_it),
                [&](const assets_tree_node &in) -> bool {
                  //              DOODLE_LOG_INFO("检查节点 {}", in.name);
                  return in.name.name == input_data.node_name;
                }
            );
            it == l_root_it.end()) {
          DOODLE_LOG_INFO("添加节点 {}", input_data.node_name);
          entt::handle const l_h{*g_reg(), g_reg()->create()};
          l_h.emplace<database>();
          l_h.emplace<assets>(input_data.node_name);

          tree_.append_child(l_root_it, assets_tree_node{input_data.node_name, l_h});
          //      edit_data = true;
          ImGui::CloseCurrentPopup();
        }
      }
    }

    if (auto l_drag_drop = dear::DragDropTarget{}) {
      if (const auto *l_drop_it = ImGui::AcceptDragDropPayload(doodle_config::drop_ass_tree_node.data()); l_drop_it) {
        auto *l_drop_it_ptr = reinterpret_cast<tree_type_t::iterator *>(l_drop_it->Data);
        boost::asio::post(g_io_context(), [this, l_it_child = *l_drop_it_ptr, l_it_p = tree_.begin()]() {
          move_node_(l_it_child, l_it_p);
          DOODLE_LOG_INFO("添加子项目 {}", l_it_child->name.name);
        });
      }
    }

    edit_data |= render_child(tree_.begin());
  }
  return edit_data;
}

void assets_tree::popen_menu(const tree_type_t::sibling_iterator &in) {
  ImGui::InputText(*input_data.input, &input_data.node_name);
  ImGui::SameLine();

  if (ImGui::Button(*input_data.node)) {
    auto l_parent = tree_type_t::parent(in);
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
      if (l_parent->handle && l_parent->handle.all_of<assets>()) l_parent->handle.get<assets>().add_child(l_h);

      tree_.insert(in, assets_tree_node{input_data.node_name, l_h});
      //      edit_data = true;
      ImGui::CloseCurrentPopup();
    }
  }
  ImGui::SameLine();

  if (ImGui::Button(*input_data.node_child)) {
    if (auto it = ranges::find_if(
            tree_type_t::begin(in), tree_type_t::end(in),
            [&](const assets_tree_node &in) -> bool { return in.name.name == input_data.node_name; }
        );
        it == in.end()) {
      DOODLE_LOG_INFO("添加节点 {}", input_data.node_name);
      entt::handle const l_h{*g_reg(), g_reg()->create()};
      l_h.emplace<database>();
      l_h.emplace<assets>(input_data.node_name);
      if (in->handle && in->handle.all_of<assets>()) in->handle.get<assets>().add_child(l_h);

      tree_.append_child(in, assets_tree_node{input_data.node_name, l_h});
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
  if (ImGui::Button(*delete_node)) {
    boost::asio::post(g_io_context(), [this, in]() {
      in->handle.destroy();
      delete_node_(in);
      tree_.erase_children(in);
      tree_.erase(in);
    });
    DOODLE_LOG_INFO("重命名节点和子节点 {}", in->name.name);
    ImGui::CloseCurrentPopup();
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
      } else if (const auto *l_drop_it = ImGui::AcceptDragDropPayload(doodle_config::drop_ass_tree_node.data());
                 l_drop_it) {
        auto *l_drop_it_ptr = reinterpret_cast<decltype(it) *>(l_drop_it->Data);
        boost::asio::post(g_io_context(), [this, l_it_child = *l_drop_it_ptr, l_it_p = it]() {
          move_node_(l_it_child, l_it_p);
          DOODLE_LOG_INFO("添加子项目 {}", l_it_child->name.name);
        });
      }
    }

    if (auto l_drop_source = dear::DragDropSource{}) {
      ImGui::SetDragDropPayload(doodle_config::drop_ass_tree_node.data(), &(it), sizeof(it));
      dear::Text(it->name.name);
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
                          return !current_select_handle || l_ass.assets_attr() == current_select_handle;
                        }) |
                        ranges::views::transform([](const decltype(l_view)::value_type &in_value_type) -> entt::handle {
                          auto &&[l_e, l_ass] = in_value_type;
                          return {*g_reg(), l_e};
                        }) |
                        ranges::to_vector;
}
void assets_tree::move_node_(
    const tree<assets_tree::assets_tree_node>::iterator &in_node,
    const tree<assets_tree::assets_tree_node>::iterator &in_parent
) {
  if (!in_node->handle) return;

  if (!(tree_.begin() == in_parent && tree_.depth(in_node) == 1)) {
    auto l_it = tree_.append_child(in_parent);
    tree_.move_ontop(l_it, in_node);
  }
  if (in_parent->handle && in_parent->handle.all_of<assets>()) {
    auto &l_ass = in_parent->handle.get<assets>();
    l_ass.add_child(in_node->handle);
  } else {
    in_node->handle.patch<assets>().set_parent({});
  }
}
}  // namespace doodle::gui