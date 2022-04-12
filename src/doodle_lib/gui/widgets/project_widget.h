//
// Created by TD on 2021/9/16.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

#include <boost/signals2.hpp>

namespace doodle {
/**
 * @brief 项目窗口
 * @image html doodle_project_windows.jpg 项目窗口
 * 这个窗口显示了项目的各种参数
 *
 */
class DOODLELIB_API project_widget
    : public gui::window_panel {
 public:
  project_widget();
  ~project_widget() override;

  constexpr static std::string_view name{"项目"};

  void init() override;
  void render() override;
  void failed() override;

  entt::handle p_c;

  boost::signals2::signal<void(const entt::handle&)> select_change;
};

namespace project_widget_ns {
constexpr auto prj_init = []() {
  entt::meta<project_widget>()
      .type()
      .prop("name"_hs, std::string{project_widget::name})
      .base<gui::window_panel>();
};
class prj_init_
    : public init_register::registrar_lambda<prj_init, 3> {};

}  // namespace project_widget_ns
}  // namespace doodle
