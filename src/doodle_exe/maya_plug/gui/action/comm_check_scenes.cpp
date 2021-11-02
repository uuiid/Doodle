//
// Created by TD on 2021/11/2.
//

#include "comm_check_scenes.h"

#include <doodle_lib/lib_warp/imgui_warp.h>

namespace doodle::maya_plug {

comm_check_scenes::comm_check_scenes()
    : command_base_tool() {
  p_show_str = make_imgui_name(
      this,
      "检查所有",
      "解锁法线"
      "检查重名"
      "检查大于四边面"
      "检查UV集",
      "去除大纲错误",
      "去除onModelChange3dc错误",
      "去除CgAbBlastPanelOptChangeCallback错误");
}
bool comm_check_scenes::render() {
  if (imgui::Button(""))

    return false;
}

}  // namespace doodle::maya_plug
