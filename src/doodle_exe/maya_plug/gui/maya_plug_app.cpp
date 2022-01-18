//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include <doodle_lib/gui/main_status_bar.h>
#include <doodle_lib/gui/main_menu_bar.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/core/core_set.h>
#include <maya_plug/gui/maya_menu_bar.h>
namespace doodle::maya_plug {

void maya_plug_app::load_windows() {
  g_main_loop().attach<maya_menu_bar>();
  g_main_loop().attach<main_status_bar>();
}
void maya_plug_app::hide_windows() {
  ::ShowWindow(p_hwnd, SW_HIDE);
}
}  // namespace doodle::maya_plug
