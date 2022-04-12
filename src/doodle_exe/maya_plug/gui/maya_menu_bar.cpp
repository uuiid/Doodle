//
// Created by TD on 2022/1/17.
//

#include "maya_menu_bar.h"
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/core/core_set.h>

#include <maya_plug/gui/action/comm_check_scenes.h>
#include <maya_plug/gui/action/comm_play_blast.h>
#include <maya_plug/gui/action/reference_attr_setting.h>
#include <maya_plug/gui/maya_plug_app.h>
#include <maya_plug/gui/action/create_sim_cloth.h>

namespace doodle::maya_plug {

void maya_menu_bar::menu_tool() {
  if (dear::MenuItem("引用工具")) {
    make_windows<reference_attr_setting>();
  }
  if (dear::MenuItem("场景检查工具"))
    make_windows<comm_check_scenes>();

  // if (dear::MenuItem("拍屏工具"))
  //   make_windows<comm_play_blast>();

  if (dear::MenuItem("qcloth布料制作"))
    make_windows<create_sim_cloth>();
}
}  // namespace doodle::maya_plug
