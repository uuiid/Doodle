//
// Created by TD on 2021/9/16.
//

#include "assets_filter_widget.h"
#include <doodle_core/metadata/metadata_cpp.h>
#include <core/tree_node.h>
#include <gui/widgets/assets_filter_widgets/filter_factory_base.h>
#include <gui/widgets/assets_filter_widgets/name_filter_factory.h>
#include <gui/widgets/assets_filter_widgets/filter_factory_template.h>
#include <gui/widgets/assets_filter_widgets/filter_base.h>

#include <utility>
namespace doodle {

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
      //      auto dis_1  = ranges::distance(l_ass.p_path);
      //      return len <= dis_1 &&
      //             ranges::all_of(
      //                 ranges::views::zip(
      //                     l_ass.p_path,
      //                     p_assets),
      //                 [](const std::tuple<FSys::path, FSys::path>& in) -> bool {
      //                   auto& [l_r, l_l] = in;
      //                   return l_r == l_l;
      //                 });
      return l_ass.p_path == p_assets;

    } else {
      return false;
    }
  };
};

class path_filters : public gui::filter_base {
 public:
  std::vector<path_filter> filters;
  explicit path_filters(std::vector<path_filter> in)
      : filters(std::move(in)) {}

  bool operator()(const entt::handle& in) const override {
    return ranges::any_of(filters, [&in](const auto& in_f) {
      return in_f(in);
    });
  }
};

class file_path_filter : public gui::filter_base {
 public:
  explicit file_path_filter(std::string in_string) : file_path_(std::move(in_string)) {}
  std::string file_path_;

  bool operator()(const entt::handle& in) const override {
    if (in.any_of<assets_file>()) {
      auto l_str = in.get<assets_file>().path_attr().generic_string();
      return boost::algorithm::icontains(l_str, file_path_);

    } else
      return false;
  }
};

class time_filter : public gui::filter_base {
 public:
  time_point_wrap p_begin;
  time_point_wrap p_end;
  explicit time_filter(
      time_point_wrap in_begin,
      time_point_wrap in_end
  )
      : p_begin(in_begin),
        p_end(in_end){};

  bool operator()(const entt::handle& in) const override {
    if (in.any_of<time_point_wrap>()) {
      auto&& l_time = in.get<time_point_wrap>();
      return l_time > p_begin && l_time < p_end;
    } else {
      return false;
    }
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
  constexpr const static ImGuiTreeNodeFlags base_flags{ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth};

  using data_type          = assets;
  using gui_cache          = gui::gui_cache<FSys::path>;
  using tree_node_type     = tree_node<gui_cache>;
  using tree_node_type_ptr = tree_node<gui_cache>::child_type;
  using popen_cache        = gui::gui_cache<std::string>;

  tree_node_type::child_type p_tree;
  popen_cache p_popen;

  void popen_menu(tree_node_type& in_node) {
    ImGui::InputText(*p_popen.gui_name, &p_popen.data);
    if (ImGui::Button("添加")) {
      if (auto it = ranges::find_if(in_node.child, [&](const tree_node_type::child_type& in) -> bool {
            return in->data.gui_name.name == p_popen.data;
          });
          it == in_node.child.end()) {
        auto k_path = in_node.data.data / p_popen.data;
        in_node.child.emplace_back(
            std::make_shared<tree_node_type>(
                gui_cache{
                    p_popen.data,
                    k_path}
            )
        );
        ImGui::CloseCurrentPopup();
      }
    }
  }

  bool is_select(const tree_node_type::child_type& in_node) const {
    return p_cur_selects.count(in_node) == 1;
  }

  void add_select(const tree_node_type::child_type& in_node) {
    if (ImGui::GetIO().KeyCtrl) {
      if (is_select(in_node)) {
        p_cur_selects.erase(in_node);
      } else {
        p_cur_selects.insert(in_node);
      }
    } else {
      p_cur_selects = {in_node};
    }
  }

  std::unique_ptr<gui::filter_base> make_filter_() override {
    std::vector<path_filter> in_list =
        p_cur_selects |
        ranges::views::transform(
            [](const tree_node_type::child_type& in_node) -> path_filter {
              return path_filter{in_node->data};
            }
        ) |
        ranges::to_vector;
    return std::make_unique<path_filters>(in_list);
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

      if (auto it = ranges::find_if(root->child, [&](const tree_node_type::child_type& in) -> bool {
            return in->data.gui_name.name == j.string();
          });
          it != root->child.end()) {
        root = it->get();
      } else {
        auto it1 = root->child.emplace_back(std::make_shared<tree_node_type>(gui_cache{j.generic_string(), l_p}));
        root     = it1.get();
      }
    }
  };

  void render_node(const tree_node_type_ptr& in_root) {
    for (auto&& i : in_root->child) {
      ImGuiTreeNodeFlags k_f{base_flags};
      if (is_select(i))
        k_f = base_flags | ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Selected;
      if (i->child.empty())
        k_f |= ImGuiTreeNodeFlags_::ImGuiTreeNodeFlags_Leaf;

      {
        dear::TreeNodeEx l_node{*i->data.gui_name, k_f};
        if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
          this->add_select(i);
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
              database::save(l_h);
            }
            // @todo 这里要写拖拽
          }
        };
        l_node&& [this, i]() {
          render_node(i);
        };
      }
    }
  }

  void init() override {
    p_tree->child.clear();
    for (auto&& [e, i] : g_reg()->view<data_type>().each()) {
      add_tree_node(p_tree.get(), i.get_path());
    }
  }

  void refresh_() override {
    for (auto&& i : p_obs) {
      auto k_h = make_handle(i);
      add_tree_node(p_tree.get(), k_h.get<data_type>().get_path());
    }
  }

 public:
  assets_filter_factory()
      : p_cur_selects(),
        p_tree(std::make_shared<tree_node_type>(gui_cache{"root"s, FSys::path{}})),
        p_popen("name"s, "null"s) {
    p_obs.connect(*g_reg(), entt::collector.update<data_type>());
  }
  std::set<tree_node_type::child_type> p_cur_selects;

  bool render() override {
    {
      dear::TreeNodeEx l_node{*p_tree->data.gui_name, ImGuiTreeNodeFlags_OpenOnArrow};
      dear::PopupContextItem{} && [this]() {
        popen_menu(*p_tree);
      };
      if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        this->p_cur_selects = {p_tree};
        this->is_edit       = true;
      }
      l_node&& [&]() {
        this->render_node(p_tree);
      };
    }
    return is_edit;
  }
};
#endif

class time_filter_factory : public gui::filter_factory_base {
  std::unique_ptr<gui::filter_base> make_filter_() override {
    if (use_begin.data && use_end.data)
      return std::make_unique<time_filter>(
          time_point_wrap{time_begin.data[0], time_begin.data[1], time_begin.data[2]},
          time_point_wrap{time_end.data[0], time_end.data[1], time_end.data[2]}
      );
    else if (use_begin.data) {
      return std::make_unique<time_filter>(
          time_point_wrap{time_begin.data[0], time_begin.data[1], time_begin.data[2]},
          time_point_wrap::max()
      );
    } else if (use_end.data) {
      return std::make_unique<time_filter>(
          time_point_wrap::min(),
          time_point_wrap{time_end.data[0], time_end.data[1], time_end.data[2]}
      );
    } else {
      return {};
    }
  }

 private:
  gui::gui_cache<bool> use_begin;
  gui::gui_cache<bool> use_end;
  gui::gui_cache<std::array<std::int32_t, 3>> time_begin;
  gui::gui_cache<std::array<std::int32_t, 3>> time_end;

 public:
  time_filter_factory()
      : use_begin("使用开始时间"s, false),
        use_end("使用结束时间"s, false),
        time_begin("开始"s, std::array<std::int32_t, 3>{0, 0, 0}),
        time_end("结束"s, std::array<std::int32_t, 3>{0, 0, 0}) {}
  bool render() override {
    if (ImGui::Checkbox(*use_begin.gui_name, &use_begin.data))
      this->is_edit = true;
    if (use_begin.data) {
      if (ImGui::InputInt3(*time_begin.gui_name, time_begin.data.data())) {
        this->is_edit = true;
      }
      dear::HelpMarker{"使用开始时间过滤任务"};
    }
    if (ImGui::Checkbox(*use_end.gui_name, &use_end.data))
      this->is_edit = true;
    if (use_end.data) {
      if (ImGui::InputInt3(*time_end.gui_name, time_end.data.data())) {
        this->is_edit = true;
      }
      dear::HelpMarker{"使用结束时间过滤任务"};
    }
    return false;
  }
  void refresh_() override {
  }
  void init() override {
    use_begin.data                     = false;
    use_begin.data                     = false;
    auto&& [l_y, l_m, l_d, l1, l2, l3] = time_point_wrap{}.compose();
    time_begin.data                    = {l_y, l_m, l_d};
    time_end.data                      = {l_y, l_m, l_d};
  }
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
    bool result{false};
    if (ImGui::InputText(*edit.gui_name, &edit.data, ImGuiInputTextFlags_EnterReturnsTrue)) {
      this->is_edit = true;
      result        = true;
    }
    dear::HelpMarker{"使用 enter 建开始搜素"};
    return result;
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
  using factory_gui_cache =
      gui::gui_cache<
          std::unique_ptr<gui::filter_factory_base>,
          gui::gui_cache_select>;

  class factory_chick : public gui::base_render {
   public:
    const bool p_chick;
    factory_gui_cache p_factory;
    factory_chick(bool use_chick, std::string&& in_name, std::unique_ptr<gui::filter_factory_base>&& in_factory_base)
        : p_chick(use_chick),
          p_factory(in_name, std::move(in_factory_base)) {}

    bool render(const entt::handle& in) override {
      bool result{false};
      bool l_refresh{false};
      if (p_chick) {
        if (ImGui::Checkbox(*p_factory.gui_name, &p_factory.select)) {
          result    = true;
          l_refresh = p_factory.select;
        }
        if (p_factory.select) {
          p_factory.data->refresh(l_refresh);
          p_factory.data->render();
        }
      } else {
        p_factory.data->refresh(l_refresh);
        result = p_factory.data->render();
        if (result)
          p_factory.select = result;
      }
      return result;
    };
  };

  impl() : p_conns(),
           p_filter_factorys(),
           p_filters(),
           p_sorts({gui::gui_cache<bool>{"名称排序"s, true}, gui::gui_cache<bool>{"集数排序"s, false}, gui::gui_cache<bool>{"反向"s, false}}) {}

  std::vector<boost::signals2::scoped_connection> p_conns;

  std::vector<factory_chick> p_filter_factorys;
  std::vector<std::unique_ptr<gui::filter_base>> p_filters;
  std::array<gui::gui_cache<bool>, 3> p_sorts;
  bool run_edit{false};
  std::string title_name_;
};

assets_filter_widget::assets_filter_widget()
    : p_impl(std::make_unique<impl>()) {
  p_impl->title_name_ = std::string{name};
}
assets_filter_widget::~assets_filter_widget() = default;

void assets_filter_widget::init() {
  p_impl->p_filter_factorys.emplace_back(false, "路径过滤"s, std::make_unique<file_path_filter_factory>());
  p_impl->p_filter_factorys.emplace_back(true, "季数过滤"s, std::make_unique<season_filter_factory>());
  p_impl->p_filter_factorys.emplace_back(true, "集数过滤"s, std::make_unique<episodes_filter_factory>());
  p_impl->p_filter_factorys.emplace_back(true, "镜头过滤"s, std::make_unique<shot_filter_factory>());
  p_impl->p_filter_factorys.emplace_back(true, "资产过滤"s, std::make_unique<assets_filter_factory>());
  p_impl->p_filter_factorys.emplace_back(true, "时间过滤"s, std::make_unique<time_filter_factory>());
  p_impl->p_filter_factorys.emplace_back(true, "制作人过滤"s, std::make_unique<gui::name_filter_factory>());
}

void assets_filter_widget::render() {
  /// 渲染数据

  bool l_is_edit{false};
  for (auto&& i : p_impl->p_filter_factorys) {
    l_is_edit |= i.render({});
  }

  ImGui::Separator();

  for (auto&& i : p_impl->p_sorts) {
    if (ImGui::Checkbox(*i.gui_name, &i.data)) {
      l_is_edit = true;
    }
  }

  if (ranges::any_of(p_impl->p_filter_factorys, [](const impl::factory_chick& in) {
        return in.p_factory.select && in.p_factory.data->is_edit;
      }) ||

      l_is_edit) {
    refresh(false);
  }
}
void assets_filter_widget::refresh(bool force) {
  if (!p_impl->run_edit)
    boost::asio::post(g_io_context(), [this, force]() {
      p_impl->run_edit = true;
      this->refresh_(force);
      p_impl->run_edit = false;
    });
}
void assets_filter_widget::refresh_(bool force) {
  p_impl->p_filters.clear();

  p_impl->p_filters = p_impl->p_filter_factorys |
                      ranges::views::filter([](const impl::factory_chick& in) -> bool {
                        return in.p_factory.select;
                      }) |
                      ranges::views::transform([](const impl::factory_chick& in) -> std::unique_ptr<gui::filter_base> {
                        return in.p_factory.data->make_filter();
                      }) |
                      ranges::views::filter([](const std::unique_ptr<gui::filter_base>& in) -> bool {
                        return (bool)in;
                      }) |
                      ranges::to_vector;

  std::vector<entt::handle> list{};

  auto l_v = g_reg()->view<database>(entt::exclude<project>);
  list     = l_v |
         ranges::views::transform([](const entt::entity& in) -> entt::handle {
           return make_handle(in);
         }) |
         ranges::views::filter([&](const entt::handle& in) -> bool {
           return ranges::all_of(
               p_impl->p_filters,
               [&](const std::unique_ptr<doodle::gui::filter_base>& in_f) {
                 return (*in_f)(in);
               }
           );
         }) |
         ranges::to_vector;

  if (p_impl->p_sorts[0].data) {
    ranges::partition(list, [](const entt::handle& in) -> bool {
      return in.any_of<assets_file>();
    });
    list |= ranges::action::stable_sort([&](const entt::handle& in_r, const entt::handle& in_l) -> bool {
      if (in_r.any_of<assets_file>() && in_l.any_of<assets_file>())
        return in_r.get<assets_file>() < in_l.get<assets_file>();
      return false;
    });
  }
  if (p_impl->p_sorts[1].data) {
    ranges::partition(list, [](const entt::handle& in) -> bool {
      return in.any_of<episodes>();
    });
    list |=
        ranges::action::stable_sort([&](const entt::handle& in_r, const entt::handle& in_l) -> bool {
          if (in_r.any_of<episodes>() && in_l.any_of<episodes>())
            return in_r.get<episodes>() < in_l.get<episodes>();
          return false;
        });
  }
  if (p_impl->p_sorts[2].data) {
    list |= ranges::action::reverse;
  }

  g_reg()->ctx().at<core_sig>().filter_handle(list);
}
const std::string& assets_filter_widget::title() const {
  return p_impl->title_name_;
}

}  // namespace doodle
