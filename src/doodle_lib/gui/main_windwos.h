//
// Created by TD on 2021/9/15.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

namespace doodle {
class project_widget;
class assets_widget;
class assets_file_widgets;
class long_time_tasks_widget;
class edit_widgets;
class DOODLELIB_API main_windows : public base_widget {

  bool_ptr p_debug_show;
  bool_ptr p_about_show;
  bool_ptr p_style_show;
  
  std::string p_title;
  std::shared_ptr<project_widget> p_prj;
  std::shared_ptr<assets_widget> p_ass;

  std::shared_ptr<base_widget> p_attr;
  std::shared_ptr<base_widget> p_long_task;
  std::shared_ptr<base_widget> p_edit_windows;
  std::shared_ptr<base_widget> p_setting;

  void main_menu_file();
  void main_menu_tool();
  void main_menu_windows();
  void main_menu_edit();

 public:
  main_windows();
  void frame_render() override;
};

}  // namespace doodle
