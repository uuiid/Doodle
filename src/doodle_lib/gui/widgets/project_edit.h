//
// Created by TD on 2022/2/7.
//

#pragma once

#include <doodle_app/gui/base/base_window.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::gui {

class DOODLELIB_API project_edit {
  class impl;
  std::unique_ptr<impl> p_i;
  void save();

 public:
  project_edit();
  ~project_edit();

  constexpr static std::string_view name{gui::config::menu_w::project_edit};

  void init();
  const std::string& title() const;
  bool render();
};

}  // namespace doodle::gui
