//
// Created by TD on 2021/9/17.
//

#pragma once

#include <DoodleLib/Gui/base_windwos.h>
#include <DoodleLib/doodle_lib_fwd.h>
#include <DoodleLib/libWarp/imgui_warp.h>

#include <boost/signals2.hpp>
namespace doodle {

class DOODLELIB_API long_time_tasks_widget : public base_widget {
  struct main_log {
    main_log()
        : p_log(),
          p_conn_list() {}
    ImGuiTextBuffer p_log;
    std::vector<boost::signals2::scoped_connection> p_conn_list;
  };

  struct info_log {
    info_log()
        : p_log(),
          p_conn_list() {}
    ImGuiTextBuffer p_log;
    std::vector<boost::signals2::scoped_connection> p_conn_list;
  };
  std::vector<long_term_ptr> task;
  long_term_ptr p_current_select;

  command_tool_ptr p_command_tool_ptr_;
  main_log p_main_log;
  info_log p_info_log;

  void link_main_log();
  void link_info_log();

 public:
  long_time_tasks_widget();
  void push_back(const long_term_ptr& in_term);

  void set_tool_widget(const command_tool_ptr& in_ptr);
  virtual void frame_render() override;
};
}  // namespace doodle
