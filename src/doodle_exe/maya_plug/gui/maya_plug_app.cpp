//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include <doodle_lib/gui/main_status_bar.h>
#include <doodle_lib/gui/main_menu_bar.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_core/core/core_set.h>
#include <maya_plug/gui/maya_layout.h>
#include <doodle_lib/gui/main_proc_handle.h>
namespace doodle::maya_plug {

void maya_plug_app::load_windows() {
  g_main_loop().attach<main_menu_bar>();
  g_main_loop().attach<main_status_bar>();
  g_main_loop().attach<maya_layout>();
}
void maya_plug_app::close_windows() {
  ::ShowWindow(p_hwnd, SW_HIDE);
}

maya_plug_app::maya_plug_app() {
  gui::main_proc_handle::get().win_close   = [this]() { this->close_windows(); };
  gui::main_proc_handle::get().win_destroy = []() {};
  self                                     = this;
}
}  // namespace doodle::maya_plug
