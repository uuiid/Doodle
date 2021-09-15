//
// Created by TD on 2021/9/15.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
namespace doodle {
using bool_ptr = std::shared_ptr<bool>;

class DOODLELIB_API main_windows {
  bool_ptr p_setting_click;
  bool_ptr p_quit;
  std::string p_title;

  void main_menu_file();
  void main_menu_tool();
 public:
  main_windows();
  void frame_render(const bool_ptr& is_show);
};

}  // namespace doodel
