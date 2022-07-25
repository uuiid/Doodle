//
// Created by TD on 2021/9/17.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API long_time_tasks_widget
    : public gui::window_panel {
  entt::handle p_current_select;

 public:
  long_time_tasks_widget();

  constexpr static std::string_view name{gui::config::menu_w::long_time_tasks};
  void init();
  void failed();
  void render() override;
};

namespace long_time_tasks_widget_ns {
constexpr auto init = []() {
  entt::meta<long_time_tasks_widget>()
      .type()
      .prop("name"_hs, std::string{long_time_tasks_widget::name})
      .base<gui::window_panel>();
};
class init_class
    : public init_register::registrar_lambda<init, 3> {};
}  // namespace long_time_tasks_widget_ns
}  // namespace doodle
