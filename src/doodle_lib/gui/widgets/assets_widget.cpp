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

#include <boost/hana/ext/std.hpp>
#include <boost/range/any_range.hpp>
namespace doodle {
class assets_widget::impl {
 public:
  static ImGuiTreeNodeFlags base_flags;

  struct shot_set : shot {
    std::set<entt::handle> p_list;
  };
  struct episodes_set : episodes {
    std::set<entt::handle> p_list;
  };

  struct season_set : season {
    std::set<episodes> p_eps;
    std::set<entt::handle> p_list;
  };

  using select_obj = std::variant<season,
                                  episodes,
                                  shot,
                                  assets>;

  std::set<assets> p_assets;
  entt::handle p_root;
  std::set<select_obj> p_all_selected;
  std::set<select_obj> p_all_old_selected;
  handle_list p_ctx_list;
  entt::observer p_season_obs;
  entt::observer p_episodes_obs;
  entt::observer p_shot_obs;
  entt::observer p_assets_obs;

  assets_widget* self;
  impl(assets_widget* in)
      : self(in),
        p_assets(),
        p_root() {
    p_season_obs.connect(*g_reg(), entt::collector.group<season>());
    p_episodes_obs.connect(*g_reg(), entt::collector.group<episodes>());
    p_shot_obs.connect(*g_reg(), entt::collector.group<shot>());
    p_assets_obs.connect(*g_reg(), entt::collector.group<assets>());
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
    p_ctx_list = in_ptr;
    k_reg->set<handle_list>(p_ctx_list);
    // auto comm  = command_list<comm_ass_eps,
    //                          comm_ass_shot,
    //                          comm_assets,
    //                          comm_ass_season,
    //                          comm_ass_ue4_create_shot>{};
  }
  void observer_() {
    for (auto& i : p_assets_obs) {
      auto k_h = make_handle(i);
      if (!std::any_of(p_ctx_list.begin(), p_ctx_list.end(), [&](auto& in) { return k_h == in; })) {
        p_ctx_list.push_back(make_handle(k_h));
      }
    }
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
    std::multimap<season, entt::handle> k_map;
    for (auto& k_ : k_v) {
      k_list.insert(k_v.get<season>(k_));
      k_map.insert(std::make_pair(k_v.get<season>(k_), make_handle(k_)));
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
          auto k_r = k_map.equal_range(k_season);
          handle_list k_list{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_list),
                         [](auto& in) {
                           return in.second;
                         });
          set_select(k_list, k_season);
        }
        k_node&& [&]() {
          auto k_r = k_map.equal_range(k_season);
          handle_list k_list{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_list),
                         [](auto& in) {
                           return in.second;
                         });
          render_eps(k_list);
        };
      }
    }
  }
  void render_eps(const handle_list& in_list) {
    std::set<episodes> k_list;
    std::multimap<episodes, entt::handle> k_map;
    for (auto& k_e : in_list) {
      if (k_e.all_of<episodes>()) {
        k_list.insert(k_e.get<episodes>());
        k_map.insert(std::make_pair(k_e.get<episodes>(), k_e));
      }
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
          auto k_r = k_map.equal_range(k_episodes);
          handle_list k_list{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_list),
                         [](auto& in) {
                           return in.second;
                         });
          set_select(k_list, k_episodes);
        }
        k_node&& [&]() {
          auto k_r = k_map.equal_range(k_episodes);
          handle_list k_list{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_list),
                         [](auto& in) {
                           return in.second;
                         });
          render_shot(k_list);
        };
      }
    }
  }
  void render_shot(const handle_list& in_list) {
    std::set<shot> k_list;
    std::multimap<shot, entt::handle> k_map;
    for (auto& k_e : in_list) {
      if (k_e.all_of<shot>()) {
        k_list.insert(k_e.get<shot>());
        k_map.insert(std::make_pair(k_e.get<shot>(), k_e));
      }
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
          auto k_r = k_map.equal_range(k_shot);
          handle_list k_list{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_list),
                         [](auto& in) {
                           return in.second;
                         });
          set_select(k_list, k_shot);
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
