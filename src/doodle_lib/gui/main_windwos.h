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

/**
 * @page  doodle_windwos 主要窗口
 * @tableofcontents
 *
 * @warning 第一次使用时请先进行基本的设置
 *
 *
 *
 * @section main_windwos_project_widget 主要窗口
 * @copydetails doodle::main_windows
 *
 * @subsection doodle_project_widget 项目窗口
 * @copydetails doodle::project_widget
 *
 * @subsubsection doodle_attr_project 编辑操作
 * @copydetails doodle::attr_project
 *
 *
 * @subsection doodle_assets_widget 资产窗口
 * @copydetails doodle::assets_widget
 *
 * @subsubsection doodle_attr_assets 编辑操作
 * @copydetails doodle::attr_assets
 *
 * @subsection doodle_assets_attr_widget 文件窗口
 * @copydetails doodle::assets_file_widgets
 *
 * @subsubsection doodle_assets_attr_widget_memu 编辑操作
 * @copydetails doodle::attr_assets_file
 *
 */

/**
 *
 * @brief 主要的程序窗口
 *
 *
 * @image html main_windows.jpg 主要窗口 width=30%
 *
 * 这个主要窗口中， 所有的操作都是依赖编辑选项卡的
 *
 * @warning 这个窗口中的主要功能时需要联网的，如果断网将会失败，只有部分功能可用
 *
 */

}  // namespace doodle
