//
// Created by TD on 2021/6/24.
//

#pragma once

#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui.hpp>
#include <nana/gui/widgets/button.hpp>
#include <nana/gui/widgets/combox.hpp>
#include <nana/gui/widgets/form.hpp>
#include <nana/gui/widgets/label.hpp>
#include <nana/gui/widgets/listbox.hpp>
#include <nana/gui/widgets/menu.hpp>
#include <nana/gui/widgets/menubar.hpp>
#include <nana/gui/widgets/spinbox.hpp>
#include <nana/gui/widgets/textbox.hpp>
#include <nana/gui/widgets/treebox.hpp>
#include <nana/gui/widgets/widget.hpp>

namespace doodle {
class setting_windows;
class project_widget;
class assets_widget;
class assets_attr_widget;

class DOODLELIB_API main_windows : public nana::form {
  nana::place p_layout;
  nana::menubar p_menubar;
  nana::menu p_menu;
  //  nana::listbox p_project_listbox;
  //  nana::treebox p_ass_tree_box;
  //  nana::listbox p_attr_listbox;
  std::shared_ptr<setting_windows> p_setting_windows;
  std::shared_ptr<project_widget> p_project_listbox;
  std::shared_ptr<assets_widget> p_ass_tree_box;
  std::shared_ptr<assets_attr_widget> p_attr_listbox;
  tool_box_menu_factory_ptr p_menu_factory;

 public:
  main_windows();
  void create_menubar();
};
}  // namespace doodle
