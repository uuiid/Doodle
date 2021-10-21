//
// Created by TD on 2021/9/15.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

namespace doodle {

class assets_widget;
class project_widget;
class setting_windows;
class assets_file_widgets;
class long_time_tasks_widget;
class edit_widgets;
class tool_box_widget;
class DOODLELIB_API main_windows : public base_widget {
 protected:
  std::string p_title;
  bool_ptr p_debug_show;
  bool_ptr p_about_show;
  bool_ptr p_style_show;



  project_widget *p_prj_;
  assets_widget *p_ass_;
  assets_file_widgets *p_attr_;
  long_time_tasks_widget *p_long_task_;
  edit_widgets *p_edit_windows_;
  setting_windows *p_setting_;
  tool_box_widget *p_tool_box_;

  std::vector<std::shared_ptr<windows_warp_base>> p_list_windwos;

  template <class T>
  T *create_windwos(bool is_show);

 protected:
  virtual void main_menu_file();
  virtual void main_menu_tool();
  virtual void main_menu_windows();
  virtual void main_menu_edit();

 public:
  main_windows();
  ~main_windows();
  void frame_render() override;
};

}  // namespace doodle
