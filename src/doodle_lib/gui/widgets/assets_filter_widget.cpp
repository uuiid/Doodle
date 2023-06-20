//
// Created by TD on 2021/9/16.
//

#include "assets_filter_widget.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/metadata/metadata_cpp.h>

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include "assets_filter_widgets/assets_tree.h"
#include "entt/entity/fwd.hpp"
#include "imgui.h"
#include <array>
#include <core/tree_node.h>
#include <cstdint>
#include <gui/widgets/assets_filter_widgets/assets_tree.h>
#include <gui/widgets/assets_filter_widgets/filter_base.h>
#include <gui/widgets/assets_filter_widgets/filter_factory_base.h>
#include <gui/widgets/assets_filter_widgets/filter_factory_template.h>
#include <gui/widgets/assets_filter_widgets/name_filter_factory.h>
#include <string>
#include <utility>
namespace doodle::gui {

class file_path_filter : public filter_base {
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

class time_filter : public filter_base {
 public:
  time_point_wrap p_begin;
  time_point_wrap p_end;
  explicit time_filter(time_point_wrap in_begin, time_point_wrap in_end) : p_begin(in_begin), p_end(in_end){};

  bool operator()(const entt::handle& in) const override {
    if (in.any_of<time_point_wrap>()) {
      auto&& l_time = in.get<time_point_wrap>();
      return l_time > p_begin && l_time < p_end;
    } else {
      return false;
    }
  };
};

class season_filter_factory : public filter_factory_t<season> {
 public:
  bool render() {
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

class episodes_filter_factory : public filter_factory_t<episodes> {
 public:
  bool render() {
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

class shot_filter_factory : public filter_factory_t<shot> {
 public:
  bool render() {
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

class time_filter_factory : public filter_factory_base {
  std::unique_ptr<filter_base> make_filter_() override {
    if (use_begin.data && use_end.data)
      return std::make_unique<time_filter>(
          time_point_wrap{time_begin.data[0], time_begin.data[1], time_begin.data[2]},
          time_point_wrap{time_end.data[0], time_end.data[1], time_end.data[2]}
      );
    else if (use_begin.data) {
      return std::make_unique<time_filter>(
          time_point_wrap{time_begin.data[0], time_begin.data[1], time_begin.data[2]}, time_point_wrap::max()
      );
    } else if (use_end.data) {
      return std::make_unique<time_filter>(
          time_point_wrap::min(), time_point_wrap{time_end.data[0], time_end.data[1], time_end.data[2]}
      );
    } else {
      return {};
    }
  }

 private:
  gui_cache<bool> use_begin;
  gui_cache<bool> use_end;
  gui_cache<std::array<std::int32_t, 3>> time_begin;
  gui_cache<std::array<std::int32_t, 3>> time_end;

 public:
  time_filter_factory()
      : use_begin("使用开始时间"s, false),
        use_end("使用结束时间"s, false),
        time_begin("开始"s, std::array<std::int32_t, 3>{0, 0, 0}),
        time_end("结束"s, std::array<std::int32_t, 3>{0, 0, 0}) {}
  bool render() {
    if (ImGui::Checkbox(*use_begin.gui_name, &use_begin.data)) this->is_edit = true;
    if (use_begin.data) {
      if (ImGui::InputInt3(*time_begin.gui_name, time_begin.data.data())) {
        this->is_edit = true;
      }
      dear::HelpMarker{"使用开始时间过滤任务"};
    }
    if (ImGui::Checkbox(*use_end.gui_name, &use_end.data)) this->is_edit = true;
    if (use_end.data) {
      if (ImGui::InputInt3(*time_end.gui_name, time_end.data.data())) {
        this->is_edit = true;
      }
      dear::HelpMarker{"使用结束时间过滤任务"};
    }
    return false;
  }
  void init() {
    use_begin.data                     = false;
    use_begin.data                     = false;
    auto&& [l_y, l_m, l_d, l1, l2, l3] = time_point_wrap{}.compose();
    time_begin.data                    = {l_y, l_m, l_d};
    time_end.data                      = {l_y, l_m, l_d};
  }
};

class file_path_filter_factory : public filter_factory_base {
 private:
  gui_cache<std::string> edit;

 public:
  file_path_filter_factory() : edit("路径过滤"s, ""s){};
  std::unique_ptr<filter_base> make_filter_() override {
    if (!edit.data.empty()) {
      return std::make_unique<file_path_filter>(edit.data);
    } else {
      return {};
    }
  }
  bool render() {
    bool result{false};
    if (ImGui::InputText(*edit.gui_name, &edit.data, ImGuiInputTextFlags_EnterReturnsTrue)) {
      this->is_edit = true;
      result        = true;
    }
    dear::HelpMarker{"使用 enter 建开始搜素"};
    return result;
  }

 protected:
  void init() { edit.data.clear(); }
};

class assets_filter_widget::impl {
 public:
  impl() = default;

  std::vector<std::unique_ptr<filter_base>> p_filters;
  assets_tree assets_tree_{};
  gui_cache_name_id add_filter{};
  struct {
    gui_cache_name_id name{"集数"};
    episodes episode{};
    bool render() {
      bool result{false};
      if (ImGui::InputInt(*name, &episode.p_episodes)) {
        result = true;
      }
      return result;
    }
  } episode_filter{};

  struct {
    gui_cache_name_id name{"镜头"};
    gui_cache_name_id name_ab{"Ab镜头"};
    shot shot_{};
    bool render() {
      bool result{false};
      if (ImGui::InputInt(*name, &shot_.p_shot)) {
        result = true;
      }
      if (auto l_com_box = dear::Combo{*name_ab, shot_.p_shot_ab.c_str()}) {
        static auto shot_enum{magic_enum::enum_names<shot::shot_ab_enum>()};
        for (auto& i : shot_enum) {
          if (imgui::Selectable(i.data(), i == shot_.p_shot_ab)) {
            shot_.p_shot_ab = i;
            result          = true;
          }
        }
      }
      return result;
    }
  } shot_filter{};

  struct {
    gui_cache_name_id ymd_name_begin{"年月日(开始)"};
    gui_cache_name_id ymd_name_end{"年月日(结束)"};
    std::array<std::int32_t, 3> begin_time{};
    std::array<std::int32_t, 3> end_time{};
    time_point_wrap begin_time_wrap{time_point_wrap::min()};
    time_point_wrap end_time_wrap{time_point_wrap::max()};
    bool render() {
      bool result{false};
      if (ImGui::InputInt3(*ymd_name_begin, begin_time.data())) {
        begin_time_wrap = time_point_wrap{begin_time[0], begin_time[1], begin_time[2]};
        result          = true;
      }
      if (ImGui::InputInt3(*ymd_name_end, end_time.data())) {
        end_time_wrap = time_point_wrap{end_time[0], end_time[1], end_time[2]};
        result        = true;
      }
      return result;
    }
  } time_filter{};

  struct {
    gui_cache_name_id name{"路径过滤"};
    std::string path{};
    bool render() {
      bool result{false};
      if (ImGui::InputText(*name, &path, ImGuiInputTextFlags_EnterReturnsTrue)) {
        result = true;
      }
      dear::HelpMarker{"使用 enter 建开始搜素"};
      return result;
    }
  } path_filter{};

  std::string title_name_;
  bool open{true};
};

assets_filter_widget::assets_filter_widget() : p_impl(std::make_unique<impl>()) {
  p_impl->title_name_ = std::string{name};
  init();
}
assets_filter_widget::~assets_filter_widget() = default;

void assets_filter_widget::init() { p_impl->assets_tree_.init_tree(); }

bool assets_filter_widget::render() {
  /// 渲染数据
  if (ImGui::Button(*p_impl->add_filter)) {
    if (auto l_menu = dear::PopupContextItem{}) {
      if (p_impl->episode_filter.render()) {
      }
      if (p_impl->shot_filter.render()) {
      }
      if (p_impl->time_filter.render()) {
      }
      if (p_impl->path_filter.render()) {
      }
    }
  }

  if (p_impl->assets_tree_.render())
    ;

  return p_impl->open;
}

void assets_filter_widget::refresh_(bool force) {
  p_impl->p_filters.clear();

  //  p_impl->p_filters =
  //      p_impl->p_filter_factorys |
  //      ranges::views::filter([](const impl::factory_chick& in) -> bool { return in.p_factory.select; }) |
  //      ranges::views::transform([](const impl::factory_chick& in) -> std::unique_ptr<gui::filter_base> {
  //        return in.p_factory.data->make_filter();
  //      }) |
  //      ranges::views::filter([](const std::unique_ptr<gui::filter_base>& in) -> bool { return (bool)in; }) |
  //      ranges::to_vector;

  std::vector<entt::handle> list{};

  auto l_v = g_reg()->view<database, assets_file>(entt::exclude<project, project_config::base_config>);
  list     = l_v | ranges::views::transform([](const entt::entity& in) -> entt::handle {
           return entt::handle{*g_reg(), in};
         }) |
         ranges::views::filter([&](const entt::handle& in) -> bool {
           return ranges::all_of(p_impl->p_filters, [&](const std::unique_ptr<doodle::gui::filter_base>& in_f) {
             return (*in_f)(in);
           });
         }) |
         ranges::to_vector;

  g_reg()->ctx().get<core_sig>().filter_handle(list);
}
const std::string& assets_filter_widget::title() const { return p_impl->title_name_; }

}  // namespace doodle::gui
