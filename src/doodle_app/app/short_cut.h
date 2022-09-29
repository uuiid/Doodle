//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_app/doodle_app_fwd.h>
#include <gui/base/base_window.h>
namespace doodle::gui {

class short_cut : public detail::windows_tick_interface {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  short_cut();
  virtual ~short_cut() override;

  bool tick() final;
};

}  // namespace doodle::gui

