//
// Created by TD on 2021/10/14.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>
namespace doodle {
/**
 * @page doodle_main_boolbox 工具窗口
 * @b 工具箱中的所有工具是可以不依赖主面板使用的独立小工具集合
 *
 *
 * @copydetails doodle::tool_box_widget
 * @tableofcontents
 *
 * 这个窗口是工具窗口， 也是基本不用联网就可以使用的几个窗口之一
 *
 * @subpage maya_tool "maya工具"
 * @copydetails doodle::comm_maya_tool
 *
 * @subpage vide_rool "视频工具"
 * @copydetails doodle::comm_create_video
 *
 * @subpage comm_import_ue "导入ue工具"
 * @copydetails doodle::comm_import_ue_files
 */

/**
 * @brief 工具箱窗口
 * @image html comm_maya_tool.jpg 工具窗口
 *
 */
class DOODLELIB_API tool_box_widget : publicprocess_t<tool_box_widget> {
  command_ptr p_command_tool_ptr_;

 public:
  tool_box_widget();
  void set_tool_widget(const command_ptr& in_ptr);

  constexpr static std::string_view name{"工具箱"};

  [[maybe_unused]] void init();
  [[maybe_unused]] void succeeded();
  [[maybe_unused]] void failed();
  [[maybe_unused]] void aborted();
  [[maybe_unused]] void update(delta_type, void* data);
};

}  // namespace doodle
