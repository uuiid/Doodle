//
// Created by TD on 2022/5/27.
//

#include "app.h"

#include <doodle_lib/gui/main_menu_bar.h>
#include <doodle_lib/gui/main_status_bar.h>

#include <doodle_lib/core/program_options.h>

#include <gui/window.h>
using namespace doodle;
void limited_app::load_windows() {
  g_main_loop()
      .attach<main_menu_bar>();
  g_main_loop()
      .attach<main_status_bar>();
  g_main_loop()
      .attach<limited_layout>();
}
