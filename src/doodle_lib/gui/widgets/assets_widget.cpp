//
// Created by TD on 2021/9/16.
//

#include "assets_widget.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/core/core_sig.h>

#include <boost/hana/ext/std.hpp>
#include <boost/range/any_range.hpp>

#include <gui/gui_ref/ref_base.h>
#include <core/tree_node.h>
#include <lib_warp/imgui_warp.h>
#include <lib_warp/entt_warp.h>

namespace doodle {

namespace gui {
class filter_factory_base::impl {
 public:
  std::vector<boost::signals2::scoped_connection> p_conns;
  bool need_init{false};
};

filter_factory_base::filter_factory_base()
    : p_i(std::make_unique<impl>()),
      is_disabled(false),
      p_obs(),
      is_edit(false) {
  connection_sig();
}

filter_factory_base::~filter_factory_base() = default;

std::unique_ptr<filter_base> filter_factory_base::make_filter() {
  is_edit = false;
  return make_filter_();
}
void filter_factory_base::refresh() {
  if (!is_disabled)
    refresh_();

  p_obs.clear();

  if (p_i->need_init) {
    this->init();
    p_i->need_init = false;
  }
}
void filter_factory_base::connection_sig() {
  auto& l_sig = g_reg()->ctx<core_sig>();

  p_i->p_conns.emplace_back(l_sig.project_begin_open.connect(
      [&](const std::filesystem::path&) {
        this->is_disabled = true;
      }));
  p_i->p_conns.emplace_back(l_sig.project_end_open.connect(
      [&](const entt::handle&, const doodle::project&) {
        this->is_disabled = false;
        p_i->need_init    = true;
      }));
  p_i->p_conns.emplace_back(l_sig.save_end.connect(
      [&](const std::vector<entt::handle>&) {
        p_i->need_init    = true;
      }));
}
}  // namespace gui

class path_filter : public gui::filter_base {
 public:
  FSys::path p_assets;
  explicit path_filter(FSys::path in_assets)
      : p_assets(std::move(in_assets)){};
  bool operator()(const entt::handle& in) const override {
    return in.all_of<assets>() && (in.get<assets>().get_path() > p_assets);
  };
};

class time_filter : public gui::filter_base {
  class fun_min {
   public:
    time_filter operator()(chrono::sys_time_pos in_time) {
      return time_filter{std::move(in_time), chrono::sys_time_pos::max()};
    }
  };
  class fun_max {
   public:
    time_filter operator()(chrono::sys_time_pos in_time) {
      return time_filter{chrono::sys_time_pos::min(), std::move(in_time)};
    }
  };

 public:
  chrono::sys_time_pos p_begin;
  chrono::sys_time_pos p_end;
  explicit time_filter(
      chrono::sys_time_pos in_begin,
      chrono::sys_time_pos in_end)
      : p_begin(std::move(in_begin)),
        p_end(std::move(in_end)){};

  constexpr const static fun_min set_for_min{};
  constexpr const static fun_max set_for_max{};

  bool operator()(const entt::handle& in) const override{

  };
};

class season_filter_factory : public gui::filter_factory_t<season> {
 public:
  bool render() {
    dear::Combo{"季数", select_name.c_str()} && [&]() {
      for (auto&& i : p_edit) {
        if (ImGui::Selectable(i.name_id.c_str())) {
          select_name  = i.name;
          p_cur_select = i;
          is_edit      = true;
        }
      }
    };
    return is_edit;
  }
};

class episodes_filter_factory : public gui::filter_factory_t<episodes> {
 public:
  bool render() {
    dear::Combo{"集数", select_name.c_str()} && [&]() {
      for (auto&& i : p_edit) {
        if (ImGui::Selectable(i.name_id.c_str())) {
          select_name  = i.name;
          p_cur_select = i;
          is_edit      = true;
        }
      }
    };
    return is_edit;
  }
};

class shot_filter_factory : public gui::filter_factory_t<shot> {
 public:
  bool render() {
    dear::Combo{"镜头", select_name.c_str()} && [&]() {
      for (auto&& i : p_edit) {
        if (ImGui::Selectable(i.name_id.c_str())) {
          select_name  = i.name;
          p_cur_select = i;
          is_edit      = true;
        }
      }
    };
    return is_edit;
  }
};

#if 1
class assets_filter_factory : public gui::filter_factory_base {
 public:
  constexpr const static ImGuiTreeNodeFlags base_flags{ImGuiTreeNodeFlags_OpenOnArrow |
                                                       ImGuiTreeNodeFlags_OpenOnDoubleClick |
                                                       ImGuiTreeNodeFlags_SpanAvailWidth};

  using data_type      = assets;
  using gui_cache      = gui::gui_cache<FSys::path>;
  using tree_node_type = tree_node<gui_cache>;

  tree_node_type p_tree;

  bool is_select(tree_node_type* in_root) {
    return in_root == p_cur_select;
  }

  std::unique_ptr<gui::filter_base> make_filter_() override {
    if (p_cur_select)
      return std::make_unique<path_filter>(p_cur_select->data.data);
    return {};
  }

  static void add_tree_node(tree_node_type* in_root, const FSys::path& in_path) {
    auto root = in_root;
    FSys::path l_p{};
    bool is_begin{true};
    for (auto&& j : in_path) {
      if (is_begin) {
        l_p      = j;
        is_begin = false;
      } else
        l_p /= j;

      if (auto it = boost::find_if(root->child, [&](const tree_node_type::child_type& in) -> bool {
            return in->data == j;
          });
          it != root->child.end()) {
        root = it->get();
      } else {
        auto it1 = root->child.emplace_back(std::make_shared<tree_node_type>(gui_cache{j.generic_string(), l_p}));
        root     = it1.get();
      }
    }
  };

  void render_node(tree_node_type* in_root) {
    for (auto&& i : in_root->child) {
      ImGuiTreeNodeFlags k_f{base_flags};
      if (is_select(i.get()))
        k_f = base_flags | ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      if (i->child.empty())
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf;

      {
        dear::TreeNodeEx l_node{i->data.name_id.c_str(), k_f};
        if (ImGui::IsItemClicked()) {
          p_cur_select  = i.get();
          this->is_edit = true;
        }

        l_node&& [this, i]() {
          render_node(i.get());
        };
      }
    }
  }

  void init() override {
    for (auto&& [e, i] : g_reg()->view<data_type>().each()) {
      add_tree_node(&p_tree, i.get_path());
    }
  }

  void refresh_() {
    for (auto&& i : p_obs) {
      auto k_h = make_handle(i);
      add_tree_node(&p_tree, k_h.get<data_type>().get_path());
    }
  }

 public:
  assets_filter_factory()
      : p_cur_select(),
        p_tree(gui_cache{"root"s, FSys::path{}}) {
    p_obs.connect(*g_reg(), entt::collector.update<data_type>());
  }
  tree_node_type* p_cur_select;

  bool render() {
    dear::TreeNode{p_tree.data.name_id.c_str()} && [&]() {
      this->render_node(&p_tree);
    };
    return is_edit;
  }
};
#endif

class time_filter_factory : public gui::filter_factory_base {
  std::unique_ptr<gui::filter_base> make_filter_() override {
  }

 public:
};

class assets_widget::impl {
 public:
  bool only_rand{false};
  std::vector<boost::signals2::scoped_connection> p_conns;

  using factory_gui_cache =
      gui::gui_cache<
          std::unique_ptr<gui::filter_factory_base>,
          gui::gui_cache_select>;

  std::vector<factory_gui_cache> p_filter_factorys;
  std::vector<std::unique_ptr<gui::filter_base>> p_filters;
  bool is_edit{false};
};

assets_widget::assets_widget()
    : p_impl(std::make_unique<impl>()) {
}
assets_widget::~assets_widget() = default;

void assets_widget::init() {
  g_reg()->set<assets_widget&>(*this);
  p_impl->p_conns.emplace_back(
      g_reg()->ctx<core_sig>().project_begin_open.connect(
          [&](const std::filesystem::path&) {
            p_impl->only_rand = true;
          }));
  p_impl->p_conns.emplace_back(
      g_reg()->ctx<core_sig>().project_end_open.connect(
          [&](const entt::handle&, const doodle::project&) {
            p_impl->only_rand = false;
          }));
  p_impl->p_filter_factorys.emplace_back("季数过滤"s, std::make_unique<season_filter_factory>());
  p_impl->p_filter_factorys.emplace_back("集数过滤"s, std::make_unique<episodes_filter_factory>());
  p_impl->p_filter_factorys.emplace_back("镜头过滤"s, std::make_unique<shot_filter_factory>());
  p_impl->p_filter_factorys.emplace_back("资产过滤"s, std::make_unique<assets_filter_factory>());
}
void assets_widget::succeeded() {
}
void assets_widget::failed() {
}
void assets_widget::aborted() {
}
void assets_widget::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  /// 加载数据
  //  if (p_impl->p_root && !p_impl->p_root.get<database_root>().is_end())
  /// 渲染数据
  dear::Disabled l_d{p_impl->only_rand};

  for (auto&& i : p_impl->p_filter_factorys) {
    if (ImGui::Checkbox(i.name_id.c_str(), &i.select))
      p_impl->is_edit = true;
    if (i.select) {
      i.data->refresh();
      i.data->render();
    }
  }

  if (boost::algorithm::any_of(p_impl->p_filter_factorys,
                               [](const impl::factory_gui_cache& in) {
                                 return in.select && in.data->is_edit;
                               }) ||
      p_impl->is_edit) {
    p_impl->is_edit = false;
    p_impl->p_filters.clear();
    boost::copy(p_impl->p_filter_factorys |
                    boost::adaptors::filtered([](const impl::factory_gui_cache& in) -> bool {
                      return in.select;
                    }) |
                    boost::adaptors::transformed([](const impl::factory_gui_cache& in)
                                                     -> std::unique_ptr<gui::filter_base> {
                      return in.data->make_filter();
                    }) |
                    boost::adaptors::filtered([](const std::unique_ptr<gui::filter_base>& in)
                                                  -> bool {
                      return (bool)in;
                    }),
                std::back_inserter(p_impl->p_filters));
    std::vector<entt::handle> list{};
    boost::copy(g_reg()->view<database>() |
                    boost::adaptors::transformed([](const entt::entity& in) -> entt::handle {
                      return make_handle(in);
                    }) |
                    boost::adaptors::filtered([&](const entt::handle& in) -> bool {
                      return boost::algorithm::all_of(
                          p_impl->p_filters,
                          [&](const std::unique_ptr<doodle::gui::filter_base>& in_f) {
                            return (*in_f)(in);
                          });
                    }),
                std::back_inserter(list));
    g_reg()->ctx<core_sig>().filter_handle(list);
  }
}

}  // namespace doodle
