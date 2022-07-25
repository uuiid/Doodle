//
// Created by TD on 2022/4/13.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

#include <doodle_lib/gui/strand_gui.h>
namespace doodle::gui {

class DOODLELIB_API layout_window
    : public base_window,
      public process_t<layout_window> {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
 public:
  layout_window();
  ~layout_window() override;
  [[nodiscard]] const std::string &title() const override;
  void init();

  void operator()() override;
  template <typename windows_type>
  void call_render() {
    boost::asio::post(
        ::doodle::make_process_adapter<windows_type>(
            strand_gui{::doodle::g_io_context().get_executor()}));
  };
};
}  // namespace doodle::gui
