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

  void clear_windows();
  void main_render();
 public:
  layout_window();
  ~layout_window() override;
  [[nodiscard]] const string &title() const override;
  void init() override;
  void succeeded() override;
  void update(const chrono::system_clock::duration &in_duration,
              void *in_data) override;
  /**
   * @brief
   * @param in_name 需要显示的name
   * @return 是否显示
   */
  std::shared_ptr<windows_proc::warp_proc> render_main(const std::string &in_name);
};
}  // namespace doodle::gui
