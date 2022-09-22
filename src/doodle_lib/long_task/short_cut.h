//
// Created by TD on 2022/2/10.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle::gui {

class short_cut : public detail::windows_tick_interface {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  short_cut();
  ~short_cut() override;

  bool tick() final;
};

}  // namespace doodle::gui
