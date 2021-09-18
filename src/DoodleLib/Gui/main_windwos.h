//
// Created by TD on 2021/9/15.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>
namespace doodle {
class project_widget;
class assets_widget;
class attribute_widgets;
class long_time_tasks_widget;
class DOODLELIB_API main_windows : public base_windows {
  bool_ptr p_setting_show;
  bool_ptr p_debug_show;
  bool_ptr p_about_show;
  bool_ptr p_style_show;
  bool_ptr p_long_task_show;
  std::string p_title;
  setting_windows_ptr p_setting;
  std::shared_ptr<project_widget> p_prj;
  std::shared_ptr<assets_widget> p_ass;
  std::shared_ptr<attribute_widgets> p_attr;
  std::shared_ptr<long_time_tasks_widget> p_long_task;


  void main_menu_file();
  void main_menu_tool();
  void main_menu_windows();
  void main_menu_edit();
 public:
  main_windows();
  void frame_render(const bool_ptr& is_show);
};

}  // namespace doodle
