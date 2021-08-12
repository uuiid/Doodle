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
/**
 * @page  doodle_windwos 主要窗口
 * @tableofcontents
 * 
 * @warning 第一次使用时请先进行基本的设置
 * 
 * @subpage doodle_windwos_setting \n
 * @subpage doodle_main_boolbox \n
 * @b 工具箱中的所有工具是可以不依赖主面板使用的独立小工具集合
 * 
 * @section main_windwos_project_widget 主要窗口
 * @copydetails doodle::main_windows
 * 
 * @subsection doodle_project_widget 项目窗口
 * @copydetails doodle::project_widget
 * 
 * @subsubsection doodle_project_widget_memu 右键操作
 * @copydetails doodle::menu_factory_project
 * 
 * 
 * @subsection doodle_assets_widget 资产窗口
 * @copydetails doodle::assets_widget
 * 
 * @subsubsection doodle_assets_widget_memu 右键操作
 * @copydetails doodle::menu_factory_assets
 * 
 * @subsection doodle_assets_attr_widget 属性窗口
 * @copydetails doodle::assets_attr_widget
 * 
 * @subsubsection doodle_assets_attr_widget_memu 右键操作
 * @copydetails doodle::menu_factory_assets_attr
 *  
 */

/**
 * @brief 主要的程序窗口
 * 
 * @image html main_windows.jpg 主要窗口 width=30%
 * 
 * 这个主要窗口中， 所有的操作都是依赖右键菜单和拖拽文件的
 * 
 * @warning 这个窗口中的主要功能时需要联网的，如果断网将会失败
 */
class DOODLELIB_API main_windows : public nana::form {
  nana::place p_layout;
  nana::menubar p_menubar;
  nana::menu p_menu;
  // nana::listbox p_project_listbox;
  // nana::treebox p_ass_tree_box;
  // nana::listbox p_attr_listbox;
  std::shared_ptr<setting_windows> p_setting_windows;
  std::shared_ptr<project_widget> p_project_listbox;
  std::shared_ptr<assets_widget> p_ass_tree_box;
  std::shared_ptr<assets_attr_widget> p_attr_listbox;
  tool_box_menu_factory_ptr p_menu_factory;

 public:
  main_windows();
  /**
   * @brief 创建菜单栏， 由初始化函数调用
   * 
   */
  void create_menubar();
};
}  // namespace doodle
