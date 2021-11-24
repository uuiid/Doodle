//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_lib/gui/main_windwos.h>

namespace doodle::maya_plug {
/**
 * @page doodle_maya_plug maya插件功能
 * @tableofcontents
 * 
 * @section maya_comm maya动作
 * @copydetails doodle::maya_plug::comm_check_scenes
 * 
 * @section maya_reference_attr_setting 引用文件标记
 * @copydetails reference_attr_setting
 * 
 */



class maya_windwos : public main_windows {
  void main_menu_tool() override;

 public:
  maya_windwos();
  void frame_render() override;
};

}  // namespace doodle::maya_plug
