//
// Created by TD on 2022/2/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle {

class DOODLELIB_API project_edit
    : public process_t<project_edit>,
      public gui::window_panel {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  project_edit();
  ~project_edit() override;
  constexpr static std::string_view name{"项目设置"};
  void init() override;
  void failed() override;
  void render() override;
};

}  // namespace doodle
