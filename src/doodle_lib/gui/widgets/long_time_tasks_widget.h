//
// Created by TD on 2021/9/17.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
#include <doodle_app/lib_warp/imgui_warp.h>

#include <boost/signals2.hpp>
namespace doodle::gui {

class DOODLELIB_API long_time_tasks_widget
    : public base_windows<
          dear::Begin,
          long_time_tasks_widget> {
  entt::handle p_current_select;
  std::string title_name_;

 public:
  long_time_tasks_widget();

  constexpr static std::string_view name{gui::config::menu_w::long_time_tasks};
  const std::string& title() const override;
  void render();
};

}  // namespace doodle::gui
