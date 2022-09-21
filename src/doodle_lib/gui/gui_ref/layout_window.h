//
// Created by TD on 2022/4/13.
//
#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/gui_ref/base_window.h>

#include <doodle_lib/gui/strand_gui.h>
namespace doodle::gui {

class DOODLELIB_API layout_window {
  class impl;
  std::unique_ptr<impl> p_i;

 protected:
  void add_windows(const std::string& in_name, process_adapter::rear_adapter_weak_ptr in_ptr);
  bool has_windows(const std::string& in_name);

 public:
  layout_window();
  virtual ~layout_window();
  virtual void init();

  bool tick();
  template <typename windows_type>
  void call_render() {
    if (!has_windows(std::string{windows_type::name})) {
      auto l_w = ::doodle::make_process_adapter<windows_type>(
          strand_gui{::doodle::g_io_context()}
      );
      add_windows(std::string{windows_type::name}, l_w.p_ptr);
      boost::asio::post(l_w);
    }
  };
};
}  // namespace doodle::gui
