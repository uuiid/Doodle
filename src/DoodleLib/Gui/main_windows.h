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
 * @defgroup  doodle_windwos 主要窗口
 * @tableofcontents
 * 
 * @b 第一次使用时请先进行基本的设置
 * 
 * @ref doodle_windwos_setting "设置窗口"
 * 
 * @image html main_windows.jpg 主要窗口 width=30%
 * 
 * @section doodle_window_menu_bar 窗口菜单栏
 * @subsection doodle_window_menu_bar_file 文件
 * @li @b 设置 @ref doodle_windwos_setting
 * @li @b 退出 退出软件
 * 
 * @subsection doodle_window_menu_bar_tool 工具箱
 * @b 工具箱中的所有工具是可以不依赖主面板使用的独立小工具集合
 * @image html doodle_main_tool.jpg 工具箱  width=30%
 * 
 * 
 * @subsubsection install_ue_plugin 安装ue插件

 * @subsubsection delete_ue_cache 删除ue缓存
 * @subsubsection modify_the_ue_cache_location 修改ue缓存位置
 */

/**
 * @brief 主要的程序窗口
 * 
 */
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
  /**
   * @brief 创建菜单栏
   * 
   */
  void create_menubar();
};
}  // namespace doodle
