//
// Created by TD on 2021/9/16.
//

#include "assets_filter_widget.h"

#include <doodle_lib/core/doodle_lib.h>

#include <doodle_lib/metadata/metadata_cpp.h>
#include <doodle_lib/core/core_sig.h>

#include <core/tree_node.h>
#include <lib_warp/entt_warp.h>
#include <lib_warp/imgui_warp.h>

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
void filter_factory_base::refresh(bool force) {
  if (!is_disabled)
    refresh_();

  p_obs.clear();

  if (p_i->need_init || force) {
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
        p_i->need_init = true;
      }));
}
//std::unique_ptr<sort_entt> sort_entt_factory_base::make_sort() {
//  return make_sort_();
//}
//void sort_entt_factory_base::refresh(bool force) {
//  is_edit = false;
//  this->refresh_();
//}

}  // namespace gui

class path_filter : public gui::filter_base {
 public:
  FSys::path p_assets;
  std::size_t len{};
  explicit path_filter(FSys::path in_assets)
      : p_assets(std::move(in_assets)),
        len(ranges::distance(p_assets)){};
  bool operator()(const entt::handle& in) const override {
    // boost::make_zip_iterator();
    //    auto l_r = ;
    if (in.all_of<assets>()) {
      auto& l_ass = in.get<assets>();
      auto dis_1  = ranges::distance(l_ass.p_path);
      return len <= dis_1 &&
             ranges::all_of(
                 ranges::views::zip(
                     l_ass.p_path,
                     p_assets),
                 [](const std::tuple<FSys::path, FSys::path>& in) -> bool {
                   auto& [l_r, l_l] = in;
                   return l_r == l_l;
                 });

    } else {
      return false;
    }

    //    return in.all_of<assets>() &&
    //           ranges::distance(p_assets) > ranges::distance(in.get<assets>().get_path()) &&
    //           ranges::all_of(
    //               ranges::views::zip(in.get<assets>().get_path(),
    //                                  p_assets),
    //               [](const std::tuple<FSys::path, FSys::path>& in) -> bool {
    //                 auto& [l_r, l_l] = in;
    //                 return l_r == l_l;
    //               });
  };
};

class file_path_filter : public gui::filter_base {
 public:
  explicit file_path_filter(const std::string& in_string) : file_path_(in_string) {}
  std::string file_path_;

  bool operator()(const entt::handle& in) const override {
    if (in.any_of<assets_file>()) {
      auto l_str = in.get<assets_file>().path.generic_string();
      return boost::algorithm::icontains(l_str, file_path_);
    } else
      return false;
  }
};

class time_filter : public gui::filter_base {
  class fun_min {
   public:
    time_filter operator()(chrono::sys_time_pos in_time) {
      return time_filter{in_time, chrono::sys_time_pos::max()};
    }
  };
  class fun_max {
   public:
    time_filter operator()(chrono::sys_time_pos in_time) {
      return time_filter{chrono::sys_time_pos::min(), in_time};
    }
  };

 public:
  chrono::sys_time_pos p_begin;
  chrono::sys_time_pos p_end;
  explicit time_filter(
      chrono::sys_time_pos in_begin,
      chrono::sys_time_pos in_end)
      : p_begin(in_begin),
        p_end(in_end){};

  constexpr const static fun_min set_for_min{};
  constexpr const static fun_max set_for_max{};

  bool operator()(const entt::handle& in) const override{

  };
};

class season_filter_factory : public gui::filter_factory_t<season> {
 public:
  bool render() override {
    dear::Combo{"季数", select_name.c_str()} && [&]() {
      for (auto&& i : p_edit) {
        if (ImGui::Selectable(*i.gui_name)) {
          select_name  = i.gui_name.name;
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
  bool render() override {
    dear::Combo{"集数", select_name.c_str()} && [&]() {
      for (auto&& i : p_edit) {
        if (ImGui::Selectable(*i.gui_name)) {
          select_name  = i.gui_name.name;
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
  bool render() override {
    dear::Combo{"镜头", select_name.c_str()} && [&]() {
      for (auto&& i : p_edit) {
        if (ImGui::Selectable(*i.gui_name)) {
          select_name  = i.gui_name.name;
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
  using popen_cache = gui::gui_cache<std::string>;
  popen_cache p_popen;

  void popen_menu(tree_node_type& in_node) {
    ImGui::InputText(*p_popen.gui_name, &p_popen.data);
    if (ImGui::Button("编辑")) {
      // in_node.data.data.replace_filename(p_popen.data);
      in_node.data = gui_cache{p_popen.data, in_node.data.data.replace_filename(p_popen.data)};
      ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("添加")) {
      if (auto it = boost::find_if(in_node.child, [&](const tree_node_type::child_type& in) -> bool {
            return in->data.gui_name.name == p_popen.data;
          });
          it == in_node.child.end()) {
        auto k_path = in_node.data.data / p_popen.data;
        in_node.child.emplace_back(
            std::make_shared<tree_node_type>(
                gui_cache{
                    p_popen.data,
                    k_path}));
        ImGui::CloseCurrentPopup();
      }
    }
  }

  bool is_select(tree_node_type* in_root) const {
    return in_root == p_cur_select.get();
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
            return in->data.gui_name.name == j;
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
        dear::TreeNodeEx l_node{*i->data.gui_name, k_f};
        if (ImGui::IsItemClicked()) {
          p_cur_select  = i;
          p_popen.data  = i->data.gui_name.name;
          this->is_edit = true;
        }
        dear::PopupContextItem{} && [this, &i]() {
          popen_menu(*i);
        };
        dear::DragDropTarget{} && [this, &i]() {
          if (auto l_hs = ImGui::AcceptDragDropPayload(doodle_config::drop_handle_list.data()); l_hs) {
            auto l_list = reinterpret_cast<std::vector<entt::handle>*>(l_hs->Data);
            for (auto&& l_h : *l_list) {
              l_h.emplace_or_replace<assets>(i->data.data);
              l_h.patch<database>(database::save);
            }
            g_reg()->ctx<assets_filter_widget>().refresh(false);
          }
        };
        l_node&& [this, i]() {
          render_node(i.get());
        };
      }
    }
  }

  void init() override {
    p_tree.child.clear();
    for (auto&& [e, i] : g_reg()->view<data_type>().each()) {
      add_tree_node(&p_tree, i.get_path());
    }
  }

  void refresh_() override {
    // for (auto&& i : p_obs) {
    //   auto k_h = make_handle(i);
    //   add_tree_node(&p_tree, k_h.get<data_type>().get_path());
    // }
  }

 public:
  assets_filter_factory()
      : p_cur_select(),
        p_tree(gui_cache{"root"s, FSys::path{}}),
        p_popen("name"s, "null"s) {
    // p_obs.connect(*g_reg(), entt::collector.update<data_type>());
  }
  tree_node_type::child_type p_cur_select;

  bool render() override {
    {
      dear::TreeNode l_node{*p_tree.data.gui_name};
      dear::PopupContextItem{} && [this]() {
        popen_menu(p_tree);
      };
      l_node&& [&]() {
        this->render_node(&p_tree);
      };
    }
    return is_edit;
  }
};
#endif

class time_filter_factory : public gui::filter_factory_base {
  std::unique_ptr<gui::filter_base> make_filter_() override {
  }

 public:
};

class file_path_filter_factory : public gui::filter_factory_base {
 private:
  gui::gui_cache<std::string> edit;

 public:
  file_path_filter_factory() : edit("路径过滤"s, ""s){};
  std::unique_ptr<gui::filter_base> make_filter_() override {
    if (!edit.data.empty()) {
      return std::make_unique<file_path_filter>(edit.data);
    } else {
      return {};
    }
  }
  bool render() override {
    if (ImGui::InputText(*edit.gui_name, &edit.data, ImGuiInputTextFlags_EnterReturnsTrue)) {
      this->is_edit = true;
    }
    dear::HelpMarker{"使用 enter 建开始搜素"};
    return false;
  }

 protected:
  void refresh_() override {
  }
  void init() override {
    edit.data.clear();
  }
};

class assets_filter_widget::impl {
 public:
  impl() : only_rand(),
           p_conns(),
           p_filter_factorys(),
           p_filters(),
           p_sorts({gui::gui_cache<bool>{"名称排序"s, true}, gui::gui_cache<bool>{"反向"s, false}}) {}

  bool only_rand{false};
  std::vector<boost::signals2::scoped_connection> p_conns;

  using factory_gui_cache =
      gui::gui_cache<
          std::unique_ptr<gui::filter_factory_base>,
          gui::gui_cache_select>;

  std::vector<factory_gui_cache> p_filter_factorys;
  std::vector<std::unique_ptr<gui::filter_base>> p_filters;
  std::array<gui::gui_cache<bool>, 2> p_sorts;
  bool run_edit{false};
};

assets_filter_widget::assets_filter_widget()
    : p_impl(std::make_unique<impl>()) {
}
assets_filter_widget::~assets_filter_widget() = default;

void assets_filter_widget::init() {
  g_reg()->set<assets_filter_widget&>(*this);
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
  p_impl->p_conns.emplace_back(
      g_reg()->ctx<core_sig>().save_begin.connect(
          [&](const std::vector<entt::handle>&) {
            p_impl->only_rand = true;
          }));
  p_impl->p_conns.emplace_back(
      g_reg()->ctx<core_sig>().save_end.connect(
          [&](const std::vector<entt::handle>&) {
            p_impl->only_rand = false;
          }));
  p_impl->p_filter_factorys.emplace_back("季数过滤"s, std::make_unique<season_filter_factory>());
  p_impl->p_filter_factorys.emplace_back("集数过滤"s, std::make_unique<episodes_filter_factory>());
  p_impl->p_filter_factorys.emplace_back("镜头过滤"s, std::make_unique<shot_filter_factory>());
  p_impl->p_filter_factorys.emplace_back("资产过滤"s, std::make_unique<assets_filter_factory>());
  p_impl->p_filter_factorys.emplace_back("路径过滤"s, std::make_unique<file_path_filter_factory>());

  //  p_impl->p_sorts = {{"名称排序"s, true}, {"反向"s, false}};
}
void assets_filter_widget::succeeded() {
  g_reg()->unset<assets_filter_widget>();
}
void assets_filter_widget::failed() {
  g_reg()->unset<assets_filter_widget>();
}
void assets_filter_widget::aborted() {
  g_reg()->unset<assets_filter_widget>();
}
void assets_filter_widget::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void* data) {
  /// 渲染数据
  dear::Disabled l_d{p_impl->only_rand};

  bool l_is_edit{false};
  for (auto&& i : p_impl->p_filter_factorys) {
    bool l_refresh{false};
    if (ImGui::Checkbox(*i.gui_name, &i.select)) {
      l_is_edit = true;
      l_refresh = i.select;
    }
    if (i.select) {
      i.data->refresh(l_refresh);
      i.data->render();
    }
  }

  ImGui::Separator();

  for (auto&& i : p_impl->p_sorts) {
    bool l_refresh{false};

    if (ImGui::Checkbox(*i.gui_name, &i.data)) {
      l_is_edit = true;
    }
  }

  if (boost::algorithm::any_of(p_impl->p_filter_factorys,
                               [](const impl::factory_gui_cache& in) {
                                 return in.select && in.data->is_edit;
                               }) ||

      l_is_edit) {
    refresh(false);
  }
}
void assets_filter_widget::refresh(bool force) {
  if (!p_impl->run_edit)
    g_main_loop().attach<one_process_t>([this, force]() {
      p_impl->run_edit = true;
      this->refresh_(force);
      p_impl->run_edit = false;
    });
}
void assets_filter_widget::refresh_(bool force) {
  p_impl->p_filters.clear();

  p_impl->p_filters = p_impl->p_filter_factorys |
                      ranges::views::filter([](const impl::factory_gui_cache& in) -> bool {
                        return in.select;
                      }) |
                      ranges::views::transform([](const impl::factory_gui_cache& in)
                                                   -> std::unique_ptr<gui::filter_base> {
                        return in.data->make_filter();
                      }) |
                      ranges::views::filter([](const std::unique_ptr<gui::filter_base>& in)
                                                -> bool {
                        return (bool)in;
                      }) |
                      ranges::to_vector;

  std::vector<entt::handle> list{};

  // list = ranges::to_vector(
  //     g_reg()->view<database>() |
  //     ranges::views::transform([](const entt::entity& in) -> entt::handle {
  //       return make_handle(in);
  //     }) |
  //     ranges::views::filter([&](const entt::handle& in) -> bool {
  //       return ranges::all_of(p_impl->p_filters, [&](const std::unique_ptr<doodle::gui::filter_base>& in_f) {
  //         return (*in_f)(in);
  //       });
  //     }));
  boost::copy(g_reg()->view<database>(entt::exclude<project>) | boost::adaptors::transformed([](const entt::entity& in) -> entt::handle {
                return make_handle(in);
              }) | boost::adaptors::filtered([&](const entt::handle& in) -> bool {
                return boost::algorithm::all_of(
                    p_impl->p_filters,
                    [&](const std::unique_ptr<doodle::gui::filter_base>& in_f) {
                      return (*in_f)(in);
                    });
              }),
              std::back_inserter(list));
  //  for (auto&& in_sort_entt : p_impl->p_sorts) {
  //    std::sort(list.begin(), list.end(),[&](const entt::handle& in_r, const entt::handle& in_l) -> bool { return (*in_sort_entt)(in_r, in_l); } );
  //  }

  //  auto l_v = ranges::views::all(g_reg()->view<database>(entt::exclude<project>));
  if (p_impl->p_sorts[0].data) {
    list |= ranges::action::stable_sort([&](const entt::handle& in_r, const entt::handle& in_l) -> bool {
      if (in_r.any_of<assets_file>() && in_l.any_of<assets_file>())
        return in_r.get<assets_file>() < in_l.get<assets_file>();
      return false;
    });
  }
  if (p_impl->p_sorts[1].data) {
    list |= ranges::action::reverse;
  }
  g_reg()->ctx<core_sig>().filter_handle(list);
}

}  // namespace doodle
