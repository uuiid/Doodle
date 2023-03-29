//
// Created by TD on 2022/8/10.
//

#pragma once
#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class all_user_view_widget {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

 public:
  all_user_view_widget();
  virtual ~all_user_view_widget();
  constexpr static std::string_view name{gui::config::menu_w::all_user_view_widget};

  const std::string& title() const;
  bool render();
};

}  // namespace doodle::gui
