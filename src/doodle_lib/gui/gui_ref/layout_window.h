//
// Created by TD on 2022/4/13.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>
namespace doodle::gui {

class DOODLELIB_API layout_window
    : public base_window,
      public process_t<layout_window> {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void call_render(const std::string &in_name);

 public:
  layout_window();
  ~layout_window() override;
  [[nodiscard]] const std::string &title() const override;
  void init() override;
  void succeeded() override;
  void update(const chrono::system_clock::duration &in_duration,
              void *in_data) override;
};
}  // namespace doodle::gui
