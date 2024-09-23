//
// Created by TD on 2021/9/16.
//

#include "assets_filter_widget.h"

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/metadata/episodes.h"
#include "doodle_core/metadata/metadata.h"
#include "doodle_core/metadata/project.h"
#include "doodle_core/metadata/season.h"
#include "doodle_core/metadata/shot.h"
#include "doodle_core/metadata/time_point_wrap.h"
#include <doodle_core/metadata/metadata_cpp.h>

#include "doodle_app/gui/base/ref_base.h"
#include "doodle_app/lib_warp/imgui_warp.h"

#include <doodle_lib/gui/widgets/derail/user_edit.h>

#include "boost/signals2/connection.hpp"

#include "assets_filter_widgets/assets_tree.h"
#include "entt/entity/fwd.hpp"
#include "fmt/compile.h"
#include "imgui.h"
#include "range/v3/view/enumerate.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <core/tree_node.h>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>
namespace doodle::gui {
template <typename T>
class filter {
 public:
  T p_data;
  explicit filter(T in_t) : p_data(std::move(in_t)){};
  bool operator()(const entt::handle& in) const { return in.all_of<T>() && in.get<T>() == p_data; }
};

class file_path_filter {
 public:
  explicit file_path_filter(std::string in_string) : file_path_(std::move(in_string)) {}
  std::string file_path_;

  bool operator()(const entt::handle& in) const {
    if (in.any_of<assets_file>()) {
      auto l_str = in.get<assets_file>().path_attr().generic_string();
      return boost::algorithm::icontains(l_str, file_path_);
    } else
      return false;
  }
};
class user_filter {
 private:
  entt::handle user_;

 public:
  explicit user_filter(const entt::handle in_user) : user_(std::move(in_user)) {}
  virtual bool operator()(const entt::handle& in) const {
    if (in.any_of<assets_file>()) {
      return in.get<assets_file>().user_attr() == user_;
    } else
      return false;
  };
};

template <>
class filter<time_point_wrap> {
 public:
  time_point_wrap p_begin;
  time_point_wrap p_end;
  explicit filter(time_point_wrap in_begin, time_point_wrap in_end) : p_begin(in_begin), p_end(in_end){};

  bool operator()(const entt::handle& in) const {
    if (in.any_of<time_point_wrap>()) {
      auto&& l_time = in.get<time_point_wrap>();
      return l_time > p_begin && l_time < p_end;
    } else {
      return false;
    }
  };
};

class assets_filter_widget::impl {
 public:
  impl() = default;

  std::tuple<
      std::unique_ptr<file_path_filter>, std::unique_ptr<filter<season>>, std::unique_ptr<filter<episodes>>,
      std::unique_ptr<filter<shot>>, std::unique_ptr<filter<time_point_wrap>>>
      p_filters;

  assets_tree assets_tree_{};
  gui_cache_name_id add_filter{"其他过滤器"};

  struct filter_fun {
    std::function<bool(const entt::handle)> filter{};
    gui_cache_name_id button_name{};
  };
  std::array<filter_fun, 6> filter_list{};

  struct {
    gui_cache_name_id name{"季数"};
    season season{};
    bool render() {
      bool result{false};
      if (ImGui::InputInt(*name, &season.p_int)) {
        result = true;
      }
      return result;
    }
  } season_filter{};

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
    std::string shot_ab_string_{};
    bool render() {
      bool result{false};
      if (ImGui::InputInt(*name, &shot_.p_shot)) {
        result = true;
      }
      if (auto l_com_box = dear::Combo{*name_ab, shot_ab_string_.c_str()}) {
        static auto shot_enum{magic_enum::enum_names<shot::shot_ab_enum>()};
        for (auto& i : shot_enum) {
          if (imgui::Selectable(i.data(), i == shot_ab_string_)) {
            shot_ab_string_ = i;
            shot_.set_shot_ab(std::string{i});
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
        clamp();
        result = true;
      }
      if (ImGui::InputInt3(*ymd_name_end, end_time.data())) {
        end_time_wrap = time_point_wrap{end_time[0], end_time[1], end_time[2]};
        clamp();
        result = true;
      }
      return result;
    }

    void clamp() {
      static auto min_time = time_point_wrap{2000, 0, 0};
      static auto max_time = time_point_wrap{2100, 0, 0};
      begin_time_wrap      = std::clamp(begin_time_wrap, min_time, max_time);
      end_time_wrap        = std::clamp(end_time_wrap, min_time, max_time);
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

  struct {
    gui_cache_name_id name{"用户过滤"};
    render::select_all_user_t select_user{};
    entt::handle select_user_handle{};
    bool render() {
      auto [l_b, l_u] = select_user.render(g_reg());
      if (l_b) {
        select_user_handle = l_u;
      }
      return l_b;
    }
  } user_filter{};

  std::string title_name_;
  bool open{true};

  std::vector<entt::handle> ass_tree_filter_list{};
  boost::signals2::scoped_connection connection{};
};

assets_filter_widget::assets_filter_widget() : p_impl(std::make_unique<impl>()) {
  p_impl->title_name_ = std::string{name};
  auto& l_sig         = g_ctx().get<core_sig>();
  p_impl->connection  = l_sig.project_end_open.connect([&]() { init(); });
  init();
}
assets_filter_widget::~assets_filter_widget() = default;

void assets_filter_widget::init() {
  p_impl->assets_tree_.init_tree();

  auto l_new                     = time_point_wrap{}.compose();
  p_impl->time_filter.begin_time = {l_new.year, l_new.month, l_new.day};
  p_impl->time_filter.end_time   = {l_new.year, l_new.month, l_new.day};
  p_impl->ass_tree_filter_list   = p_impl->assets_tree_.get_filter_list_handles();
}

bool assets_filter_widget::render() {
  /// 渲染数据
  if (p_impl->path_filter.render()) {
    if (p_impl->path_filter.path.empty()) {
      p_impl->filter_list[0] = {};
    } else {
      p_impl->filter_list[0] = {
          file_path_filter{p_impl->path_filter.path},
          gui_cache_name_id{fmt::format("路径过滤:{} ", p_impl->path_filter.path)}
      };
    }
    filter_list();
  }
  if (auto l_ = dear::CollapsingHeader{*p_impl->add_filter}) {
    if (p_impl->season_filter.render()) {
      p_impl->filter_list[1] = {
          filter<season>{p_impl->season_filter.season},
          gui_cache_name_id{fmt::format("季数:{} ", p_impl->season_filter.season)}
      };
      filter_list();
    }
    if (p_impl->episode_filter.render()) {
      p_impl->filter_list[2] = {
          filter<episodes>{p_impl->episode_filter.episode},
          gui_cache_name_id{fmt::format("集数:{} ", p_impl->episode_filter.episode)}
      };
      filter_list();
    }
    if (p_impl->shot_filter.render()) {
      p_impl->filter_list[3] = {
          filter<shot>{p_impl->shot_filter.shot_}, gui_cache_name_id{fmt::format("镜头:{} ", p_impl->shot_filter.shot_)}
      };
      filter_list();
    }
    if (p_impl->time_filter.render()) {
      p_impl->filter_list[4] = {
          filter<time_point_wrap>{p_impl->time_filter.begin_time_wrap, p_impl->time_filter.end_time_wrap},
          gui_cache_name_id{fmt::format("时间:{}-{} ", p_impl->time_filter.begin_time_wrap, p_impl->time_filter.end_time_wrap)}};
      filter_list();
    }
    if (p_impl->user_filter.render()) {
      p_impl->filter_list[5] = {
          user_filter{p_impl->user_filter.select_user_handle},
          gui_cache_name_id{fmt::format(
              "用户:{}(id {}) ",
              p_impl->user_filter.select_user_handle ? p_impl->user_filter.select_user_handle.get<user>() : user{},
              (p_impl->user_filter.select_user_handle && p_impl->user_filter.select_user_handle.any_of<database>())
                  ? p_impl->user_filter.select_user_handle.get<database>().get_id()
                  : 0
          )}};
      filter_list();
    }
  }

  for (auto&& l_data : p_impl->filter_list) {
    if (l_data.filter) {
      if (ImGui::Button(*l_data.button_name)) {
        l_data.filter = {};
        filter_list();
      }
    }
  }
  if (p_impl->assets_tree_.render()) {
    p_impl->ass_tree_filter_list = p_impl->assets_tree_.get_filter_list_handles();
    filter_list();
  }

  return p_impl->open;
}

void assets_filter_widget::tree_refresh() {
  p_impl->assets_tree_.init_tree();
  p_impl->ass_tree_filter_list = p_impl->assets_tree_.get_filter_list_handles();
}

const std::string& assets_filter_widget::title() const { return p_impl->title_name_; }
void assets_filter_widget::filter_list() {
  std::vector<entt::handle> list{};

  list =
      p_impl->ass_tree_filter_list | ranges::views::filter([&](const entt::handle& in) -> bool {
        return ranges::all_of(p_impl->filter_list, [&](const decltype(p_impl->filter_list)::value_type& in_f) -> bool {
          return !in_f.filter || in_f.filter(in);
        });
      }) |
      ranges::to_vector;

  g_ctx().get<core_sig>().filter_handle(list);
}

}  // namespace doodle::gui
