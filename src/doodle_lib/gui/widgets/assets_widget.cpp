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

  using select_obj = std::variant<season,
                                  episodes,
                                  shot,
                                  string>;

  std::deque<string> p_select_curr_path_;
  std::deque<string> p_select_path_;

  struct select_guard {
    impl* self;
    select_guard(impl* in_self, const string& in_str)
        : self(in_self) {
      self->p_select_curr_path_.push_back(in_str);
    };
    ~select_guard() {
      self->p_select_curr_path_.pop_back();
    };
  };

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
        p_root() {
    p_season_obs.connect(*g_reg(), entt::collector.group<season>());
    p_episodes_obs.connect(*g_reg(), entt::collector.group<episodes>());
    p_shot_obs.connect(*g_reg(), entt::collector.group<shot>());
    p_assets_obs.connect(*g_reg(), entt::collector.group<assets>());
  };

  bool is_select() {
    return p_select_curr_path_ == p_select_path_;
    // return std::any_of(p_all_old_selected.begin(), p_all_old_selected.end(),
    //                    [&](const select_obj& in_) {
    //                      return in_ == in_ptr;
    //                    });
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
    p_select_path_ = p_select_curr_path_;
    // p_all_selected.clear();
    // p_all_selected.insert(in_obj);
    auto k_reg = g_reg();
    p_ctx_list = in_ptr;
    k_reg->set<handle_list>(p_ctx_list);
    // auto comm  = command_list<comm_ass_eps,
    //                          comm_ass_shot,
    //                          comm_assets,
    //                          comm_ass_season,
    //                          comm_ass_ue4_create_shot>{};
  }

  void observer_main() {
    observer_(p_season_obs);
    observer_(p_episodes_obs);
    observer_(p_shot_obs);
    observer_(p_assets_obs);
  }
  void observer_(entt::observer& in_obs) {
    /// 检查创建的大小， 太多不显示， 之间清除
    if (in_obs.size() > 10) {
      in_obs.clear();
      return;
    }
    bool set_ctx{false};
    for (auto i : in_obs) {
      auto k_h = make_handle(i);
      /// 开始测试是否在选择中包含
      if (!std::any_of(p_ctx_list.begin(), p_ctx_list.end(), [&](auto& in) { return k_h == in; })) {
        set_ctx = true;
        p_ctx_list.push_back(make_handle(k_h));
      }
    }
    /// 测试成功后添加到上下文中
    if (set_ctx) {
      g_reg()->set<handle_list>(p_ctx_list);
    }

    in_obs.clear();
  }

  /**
   * @brief  加载树节点
   *
   */
  void render() {
    render_season();
    render_assets();
    observer_main();

    p_all_old_selected = p_all_selected;
  }

  void render_assets() {
    handle_list k_list{};
    for (auto& in : g_reg()->view<assets>(entt::exclude<season, episodes, shot>)) {
      k_list.push_back(make_handle(in));
    }
    render_assets_recursion(k_list, 0);
  }

  void render_assets_recursion(const std::vector<entt::handle>& in_list, std::size_t in_deep) {
    if (in_list.empty())
      return;

    std::set<string> k_list{};
    std::multimap<string, entt::handle> k_map{};
    bool is_leaf{true};

    for (auto& k_h : in_list) {
      if (!k_h.all_of<assets>())
        continue;
      auto& k_ass = k_h.get<assets>();
      if (k_ass.get_path_component().size() <= in_deep)
        continue;

      is_leaf &= ((k_ass.get_path_component().size() - 1) == in_deep);
      k_list.insert(k_ass.get_path_component()[in_deep]);
      k_map.insert(std::make_pair(k_ass.get_path_component()[in_deep], k_h));
    }

    for (auto& k_ass_str : k_list) {
      auto k_f = base_flags;
      select_guard k_guard{this, k_ass_str};

      if (is_select())
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

      if (is_leaf)
        k_f = k_f | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
      {
        dear::TreeNodeEx k_node{k_ass_str.c_str(),
                                k_f,
                                k_ass_str.c_str()};
        if (imgui::IsItemClicked()) {
          auto k_r = k_map.equal_range(k_ass_str);
          handle_list k_h_list{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_h_list),
                         [](auto& in) {
                           return in.second;
                         });
          set_select(k_h_list, k_ass_str);
        }
        k_node&& [&]() {
          auto k_r = k_map.equal_range(k_ass_str);
          handle_list k_h_list{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_h_list),
                         [](auto& in) {
                           return in.second;
                         });
          if (k_h_list.empty())
            return;
          render_assets_recursion(k_h_list, in_deep + 1);
        };
      }
    }
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
      select_guard k_guard{this, k_season.str()};

      if (is_select())
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
      select_guard k_guard{this, k_episodes.str()};

      if (is_select())
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
      {
        select_guard k_guard{this, k_shot.str()};
        if (is_select())
          k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

        handle_list k_list{};
        boost::copy(
            k_map.equal_range(k_shot) |
                boost::adaptors::transformed([](auto& k_p) {
                  return k_p.second;
                }) |
                boost::adaptors::filtered([](const entt::handle& in) {
                  return in.all_of<assets>();
                }),
            std::back_inserter(k_list));
        if (k_list.empty())
          k_f = k_f | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        dear::TreeNodeEx k_node{
            k_shot.str().c_str(),
            k_f,
            k_shot.str().c_str()};
        if (imgui::IsItemClicked()) {
          auto k_r = k_map.equal_range(k_shot);
          handle_list k_list_s{};
          std::transform(k_r.first, k_r.second, std::back_inserter(k_list_s),
                         [](auto& in) {
                           return in.second;
                         });
          set_select(k_list_s, k_shot);
        }

        k_node&& [&]() {
          render_assets_recursion(k_list, 0);
        };
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
