//
// Created by TD on 2021/9/15.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Gui/base_windwos.h>
namespace doodle {
class project_widget;
class DOODLELIB_API main_windows : public base_windows {
  bool_ptr p_setting_show;
  bool_ptr p_debug_show;
  bool_ptr p_about_show;
  bool_ptr p_style_show;
  bool_ptr p_quit;
  std::string p_title;
  setting_windows_ptr p_setting;
  std::shared_ptr<project_widget> p_prj;

  void main_menu_file();
  void main_menu_tool();

 public:
  main_windows();
  void frame_render(const bool_ptr& is_show);
};

}  // namespace doodle
