//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include <doodle_lib/gui/main_status_bar.h>
#include <doodle_lib/gui/main_menu_bar.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/long_task/process_pool.h>
#include <doodle_lib/core/core_set.h>
namespace doodle::maya_plug {
maya_plug_app::maya_plug_app()
    : doodle_app() {
}
void maya_plug_app::load_windows() {
  g_main_loop().attach<main_menu_bar>();
  g_main_loop().attach<main_status_bar>();
  g_main_loop().attach<null_process_t>().then([](auto, auto, auto s, auto) {
    core_set::getSet().load_first_project();
    s();
  });
}
}  // namespace doodle::maya_plug
