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
class assets_widget::impl {
 public:
  static ImGuiTreeNodeFlags base_flags;

  using select_obj = std::variant<season,
                                  episodes,
                                  shot,
                                  assets>;

  std::set<assets> p_assets;
  entt::handle p_root;
  std::set<select_obj> p_all_selected;
  std::set<select_obj> p_all_old_selected;

  assets_widget* self;
  impl(assets_widget* in)
      : self(in),
        p_assets(),
        p_root(){
            // p_season_obs.connect(*g_reg(), entt::collector.group<season>());
            // p_episodes_obs.connect(*g_reg(), entt::collector.group<episodes>());
            // p_shot_obs.connect(*g_reg(), entt::collector.group<shot>());
            // p_assets_obs.connect(*g_reg(), entt::collector.group<assets>());
        };

  bool is_select(const select_obj& in_ptr) {
    return std::any_of(p_all_old_selected.begin(), p_all_old_selected.end(),
                       [&](const select_obj& in_) {
                         return in_ == in_ptr;
                       });
  }

  void check_item(const select_obj& in_ptr) {
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
  void check_item_clicked(const select_obj& in_ptr) {
    if (imgui::IsItemClicked()) {
      if (!imgui::GetIO().KeyCtrl) {
        p_all_selected.clear();
        p_all_old_selected.clear();
      } else {
        p_all_selected.erase(in_ptr);
        return;
      }
    }
  }

  void set_select(const std::vector<entt::handle>& in_ptr, const select_obj& in_obj) {
    p_all_selected.clear();
    p_all_selected.insert(in_obj);
    auto k_reg = g_reg();
    k_reg->set<handle_list>(in_ptr);
    // auto comm  = command_list<comm_ass_eps,
    //                          comm_ass_shot,
    //                          comm_assets,
    //                          comm_ass_season,
    //                          comm_ass_ue4_create_shot>{};
  }
  /**
   * @brief  加载树节点
   *
   */
  void render() {
    render_season();
    p_all_old_selected = p_all_selected;
  }

  void render_season() {
    auto k_g = g_reg();
    auto k_v = k_g->view<season>();
    std::set<season> k_list;
    for (auto& k_ : k_v) {
      k_list.insert(k_v.get<season>(k_));
    }

    for (auto& k_season : k_list) {
      auto k_f = base_flags;

      if (is_select(k_season))
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

      {
        dear::TreeNodeEx k_node{k_season.str().c_str(),
                                k_f,
                                k_season.str().c_str()};

        if (imgui::IsItemClicked()) {
          std::vector<entt::handle> k_handle_list{};
          for (auto& k_i : k_v) {
            if (k_v.get<season>(k_i) == k_season)
              k_handle_list.push_back(make_handle(k_i));
          }
          set_select(k_handle_list, k_season);
        }
        k_node&& [&]() {
          render_eps(k_season);
        };
      }
    }
  }
  void render_eps(const season& in_season) {
    auto k_g = g_reg();
    auto k_v = k_g->view<episodes>();
    std::set<episodes> k_list;
    for (auto& k_e : k_v) {
      auto k_h = make_handle(k_e);
      if (k_h.all_of<season>() && k_h.get<season>() == in_season)
        k_list.insert(k_v.get<episodes>(k_e));
    }

    for (auto& k_episodes : k_list) {
      auto k_f = base_flags;

      if (is_select(k_episodes))
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      {
        dear::TreeNodeEx k_node{k_episodes.str().c_str(),
                                k_f,
                                k_episodes.str().c_str()};
        if (imgui::IsItemClicked()) {
          std::vector<entt::handle> k_handle_list{};
          for (auto& k_i : k_v) {
            auto k_h = make_handle(k_i);
            if (k_v.get<episodes>(k_i) == k_episodes &&
                k_h.all_of<season>() &&
                k_h.get<season>() == in_season)
              k_handle_list.push_back(make_handle(k_i));
          }
          set_select(k_handle_list, k_episodes);
        }
        k_node&& [&]() {
          render_shot(k_episodes, in_season);
        };
      }
    }
  }
  void render_shot(const episodes& in_eps, const season& in_season) {
    auto k_g = g_reg();
    auto k_v = k_g->view<shot>();
    std::set<shot> k_list;
    for (auto& k_e : k_v) {
      auto k_h = make_handle(k_e);
      if (k_h.all_of<episodes>() &&
          k_h.get<episodes>() == in_eps &&
          k_h.all_of<season>() &&
          k_h.get<season>() == in_season)
        k_list.insert(k_v.get<shot>(k_e));
    }

    for (auto& k_shot : k_list) {
      auto k_f = base_flags;
      if (is_select(k_shot))
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      {
        dear::TreeNodeEx k_node{
            k_shot.str().c_str(),
            k_f | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
            k_shot.str().c_str()};
        if (imgui::IsItemClicked()) {
          std::vector<entt::handle> k_handle_list{};
          for (auto& k_i : k_v) {
            auto k_h = make_handle(k_i);
            if (k_v.get<shot>(k_i) == k_shot &&
                k_h.all_of<episodes>() &&
                k_h.get<episodes>() == in_eps &&
                k_h.all_of<season>() &&
                k_h.get<season>() == in_season)
              k_handle_list.push_back(make_handle(k_i));
          }
          set_select(k_handle_list, k_shot);
        }
      }
    }
  }
};
ImGuiTreeNodeFlags assets_widget::impl::base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                   ImGuiTreeNodeFlags_SpanAvailWidth};

assets_widget::assets_widget()
    : p_impl(std::make_unique<impl>(this)) {
  p_factory    = new_object<attr_assets>();
  p_class_name = "资产";
}
assets_widget::~assets_widget() = default;

void assets_widget::frame_render() {
  /// 加载数据
  if (p_impl->p_root && !p_impl->p_root.get<database_root>().is_end())
    p_impl->p_root.patch<database_stauts>(database_set_stauts<need_root_load>{});
  /// 渲染数据
  p_impl->render();
}
std::vector<entt::handle> assets_widget::get_selects() const {
  return {};
}

void assets_widget::set_metadata(const entt::entity& in_ptr) {
  auto k_h = make_handle(in_ptr);
  if (!k_h.all_of<database_root, database, database_stauts>())
    throw doodle_error{"缺失组件"};
  p_impl->p_root = k_h;
}

}  // namespace doodle
