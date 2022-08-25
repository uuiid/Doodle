//
// Created by TD on 2022/8/10.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>

#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle::gui {

namespace all_user_view_widget_ns {

}

class all_user_view_widget : public window_panel {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  all_user_view_widget();
  virtual ~all_user_view_widget();
  constexpr static std::string_view name{gui::config::menu_w::all_user_view_widget};

 protected:
  virtual void render() override;
};

}  // namespace doodle::gui
