//
// Created by TD on 2021/8/4.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

#include <nana/gui/widgets/menu.hpp>

namespace doodle {
/**
 * @page doodle_main_boolbox 菜单栏
 * @section doodle_window_menu_bar_file 文件
 * @li @b 设置 @ref doodle_windwos_setting
 * @li @b 退出 退出软件
 * 
 * @section doodle_window_menu_bar_tool 工具箱
 * @image html doodle_main_tool.jpg 工具箱  width=30%
 * 
 * @subsection doodle_toolbox_actn_export_maya 导出fbx
 * @copydetails doodle::toolbox::actn_export_maya
 * 
 * @subsection create_video 创建视频
 * @copydetails doodle::toolbox::actn_create_video
 * 
 * @subsection connection_video 连接视频
 * @copydetails doodle::toolbox::actn_connect_video
 * 
 * @subsection create_ue_level 创建ue关卡
 * @copydetails doodle::toolbox::actn_ue4_shot_episodes
 * 
 * @subsection install_ue_plugin 安装ue插件
 * @subsection install_ue_project_plugin 安装ue项目插件
 * @copydetails doodle::toolkit::installUePath
 * 
 * @subsection installMayaPath 安装maya插件
 * @copydetails doodle::toolkit::installMayaPath
 * 
 * @subsection modifyUeCachePath 修改ue4缓存位置
 * @copydetails doodle::toolkit::modifyUeCachePath
 * 
 * @subsection deleteUeCache 删除ue4缓存
 * @copydetails doodle::toolkit::deleteUeCache
 */

/**
 * @brief 这个是用来创建工具菜单栏的工厂
 * 
 */
class DOODLELIB_API tool_box_menu_factory
    : public std::enable_shared_from_this<tool_box_menu_factory> {
  std::vector<action_base_ptr> p_list;
  nana::window p_window;

  void create_menu();

 public:
  tool_box_menu_factory(nana::window in_window);

  void operator()(nana::menu& in_menu);
};
}  // namespace doodle
