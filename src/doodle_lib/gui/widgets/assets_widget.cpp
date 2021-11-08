//
// Created by TD on 2021/9/16.
//

#include "assets_widget.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_meta.h>
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
  /// 加载数据
  if (p_root && !p_root.get<database_root>().is_end())
    p_root.patch<database_stauts>(database_set_stauts<need_root_load>{});
  load_meta(p_root);
  p_all_old_selected = p_all_selected;
}

void assets_widget::set_metadata(const entt::entity& in_ptr) {
  auto k_h = make_handle(in_ptr);
  if (!k_h.all_of<database_root, database, database_stauts>())
    throw doodle_error{"缺失组件"};
  p_root = k_h;
  if (!p_root.get<database_root>().is_end())
    p_root.patch<database_stauts>(database_set_stauts<need_root_load>{});
}

class assets_widget::node {
 public:
  ImGuiTreeNodeFlags flage;
  string show_str;
  string uuid;
  entt::handle ent;

  template <class T>
  static std::vector<T> load_node(assets_widget* self) {
    static auto base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                           ImGuiTreeNodeFlags_OpenOnDoubleClick |
                           ImGuiTreeNodeFlags_SpanAvailWidth};
    auto k_reg = g_reg();
    std::vector<node> k_list;
    for (auto& k_s : k_reg->view<T>()) {
      node k_node{};
      auto k_h   = make_handle(k_s);
      k_node.ent = k_h;
      if (self->is_select(k_h)) {
        k_node.flage |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      }
      k_node.show_str = k_h.get_or_emplace<to_str>().get();
      k_node.uuid     = k_h.get<database>().get_uuid();
      k_list.push_back(std::move(k_node));
    }
    return k_list;
  }

  static void load_(assets_widget* self,
                    const std::vector<node>& in_list,
                    std::function<void()>&& in_fun) {
    for (auto& k_n : in_list) {
      bool checked = false;
      dear::TreeNodeEx{
          k_n.uuid.c_str(),
          k_n.flage,
          k_n.show_str.c_str()} &&
          [&]() {
            if (!checked) {
              self->check_item_clicked(k_n.ent);
              checked = true;
            }
            if (in_fun)
              in_fun();
          };
      if (!checked)
        self->check_item_clicked(k_n.ent);
      self->check_item(k_n.ent);
    }
  }
};

void assets_widget::load_meta(const entt::handle& in_ptr) {
  static auto base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                         ImGuiTreeNodeFlags_OpenOnDoubleClick |
                         ImGuiTreeNodeFlags_SpanAvailWidth};

  auto k_season   = node::load_node<season>(this);
  auto k_episodes = node::load_node<episodes>(this);
  auto k_shot     = node::load_node<shot>(this);
  auto k_assets   = node::load_node<assets>(this);


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
  p_meta     = in_ptr;
  auto k_reg = g_reg();
  auto comm  = command_list<comm_ass_eps,
                           comm_ass_shot,
                           comm_assets,
                           comm_ass_season,
                           comm_ass_ue4_create_shot>{};
  comm.set_data(p_meta);
  k_reg->set<widget_>(comm);

  p_all_selected.insert(p_meta);
  select_change(p_meta);
}
}  // namespace doodle
