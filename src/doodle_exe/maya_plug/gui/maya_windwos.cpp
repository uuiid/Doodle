//
// Created by TD on 2021/10/14.
//

#include "maya_windwos.h"

#include <doodle_lib/gui/widgets/tool_box_widget.h>
#include <maya/MFileIO.h>
#include <maya_plug/gui/action/reference_attr_setting.h>
#include <maya_plug/gui/maya_plug_app.h>
namespace doodle::maya_plug {

void maya_windwos::main_menu_tool() {
  main_windows::main_menu_tool();
  auto k_task = win_cast<tool_box_widget>(p_tool_box);
  if (dear::MenuItem("maya引用工具") && k_task)
    k_task->set_tool_widget(new_object<reference_attr_setting>());
}

maya_windwos::maya_windwos()
    : main_windows() {
  p_title = fmt::format("{} VM {}##{}", p_title, MAYA_API_VERSION, p_title);
}

void maya_windwos::frame_render() {
  main_windows::frame_render();
}
}  // namespace doodle::maya_plug