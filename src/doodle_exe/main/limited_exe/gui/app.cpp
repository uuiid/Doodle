//
// Created by TD on 2022/5/27.
//

#include "app.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/thread_pool/asio_pool.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/gui/main_menu_bar.h>
#include <doodle_lib/gui/main_status_bar.h>
#include <doodle_lib/gui/gui_ref/layout_window.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/platform/win/drop_manager.h>
#include <doodle_lib/long_task/short_cut.h>
#include <doodle_lib/core/image_loader.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/core/init_register.h>
#include <doodle_lib/gui/main_proc_handle.h>
#include <doodle_lib/gui/get_input_dialog.h>

#include <doodle_lib/core/program_options.h>
#include <doodle_lib/lib_warp/icon_font_macro.h>

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
