//
// Created by TD on 2022/4/21.
//
#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

namespace doodle::gui {

class DOODLELIB_API subtitle_processing : public window_panel {
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  subtitle_processing();
  ~subtitle_processing() override;

 protected:
  virtual void render() override;
};

}  // namespace doodle::gui
