//
// Created by TD on 2022/1/17.
//

#include "maya_menu_bar.h"
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/core/core_set.h>

#include <maya_plug/gui/action/comm_check_scenes.h>
#include <maya_plug/gui/action/comm_play_blast.h>
#include <maya_plug/gui/action/reference_attr_setting.h>
#include <maya_plug/gui/maya_plug_app.h>
#include <maya_plug/gui/action/create_sim_cloth.h>

#include <boost/hana/ext/std/tuple.hpp>
#include <maya_plug/configure/static_value.h>
namespace doodle::maya_plug {

void maya_menu_bar::menu_windows() {
  std::apply([this](const auto&... in_item) {
    (this->widget_menu_item(in_item), ...);
  },
             gui::config::maya_plug::menu::menu_maya);
}
}  // namespace doodle::maya_plug
