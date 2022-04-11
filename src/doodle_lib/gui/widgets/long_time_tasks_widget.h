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
    : public process_t<long_time_tasks_widget>,
      public gui::window_panel {
  entt::handle p_current_select;

 public:
  long_time_tasks_widget();

  constexpr static std::string_view name{"队列"};
  void init() override;
  void failed() override;
  void render() override;
};
}  // namespace doodle
