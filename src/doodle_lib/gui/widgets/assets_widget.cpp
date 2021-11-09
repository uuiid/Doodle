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

  std::set<season> p_season;
  std::set<episodes> p_episodes;
  std::set<shot> p_shot;
  std::set<assets> p_assets;
  entt::handle p_root;
  select_obj p_select;
  std::set<select_obj> p_all_selected;
  std::set<select_obj> p_all_old_selected;

  entt::observer p_season_obs;
  entt::observer p_episodes_obs;
  entt::observer p_shot_obs;
  entt::observer p_assets_obs;

  assets_widget* self;
  impl(assets_widget* in)
      : self(in),
        p_season(),
        p_episodes(),
        p_shot(),
        p_assets(),
        p_root() {
    p_season_obs.connect(*g_reg(), entt::collector.group<season>());
    p_episodes_obs.connect(*g_reg(), entt::collector.group<episodes>());
    p_shot_obs.connect(*g_reg(), entt::collector.group<shot>());
    p_assets_obs.connect(*g_reg(), entt::collector.group<assets>());
  };

  void load() {
    auto k_reg = g_reg();
    {
      for (auto& k_s : p_season_obs) {
        p_season.insert(make_handle(k_s).get<season>());
      }
    }
    {
      for (auto& k_s : p_episodes_obs) {
        p_episodes.insert(make_handle(k_s).get<episodes>());
      }
    }
    {
      for (auto& k_s : p_shot_obs) {
        p_shot.insert(make_handle(k_s).get<shot>());
      }
    }
    p_season_obs.clear();
    p_episodes_obs.clear();
    p_shot_obs.clear();
  }

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
      set_select(in_ptr);
    }
  }

  void set_select(const select_obj& in_ptr) {
    p_select = in_ptr;
    // auto k_reg = g_reg();
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
    load();
    render_season();
  }

  void render_season() {
    for (auto& k_season : p_season) {
      auto k_f     = base_flags;
      bool checked = false;
      if (is_select(k_season))
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;

      dear::TreeNodeEx{k_season.str().c_str(),
                       k_f,
                       k_season.str().c_str()} &&
          [&]() {
            if (!checked) {
              check_item_clicked(k_season);
              checked = true;
            }
            render_eps();
          };
      if (!checked)
        check_item_clicked(k_season);
      check_item(k_season);
    }
  }
  void render_eps() {
    for (auto& k_episodes : p_episodes) {
      auto k_f     = base_flags;
      bool checked = false;
      if (is_select(k_episodes))
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      dear::TreeNodeEx{k_episodes.str().c_str(),
                       k_f,
                       k_episodes.str().c_str()} &&
          [&]() {
            if (!checked) {
              check_item_clicked(k_episodes);
              checked = true;
            }
            render_shot();
          };
      if (!checked)
        check_item_clicked(k_episodes);
      check_item(k_episodes);
    }
  }
  void render_shot() {
    for (auto& k_shot : p_shot) {
      auto k_f = base_flags;
      if (is_select(k_shot))
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      dear::TreeNodeEx{
          k_shot.str().c_str(),
          k_f | ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen,
          k_shot.str().c_str()};
      check_item_clicked(k_shot);
      check_item(k_shot);
    }
  }
};
ImGuiTreeNodeFlags assets_widget::impl::base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                                                   ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                   ImGuiTreeNodeFlags_SpanAvailWidth};

assets_widget::assets_widget()
    : p_meta(),
      p_all_old_selected(),
      p_all_selected(),
      p_impl(std::make_unique<impl>(this)) {
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
  p_all_old_selected = p_all_selected;
}

void assets_widget::set_metadata(const entt::entity& in_ptr) {
  auto k_h = make_handle(in_ptr);
  if (!k_h.all_of<database_root, database, database_stauts>())
    throw doodle_error{"缺失组件"};
  p_impl->p_root = k_h;
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
