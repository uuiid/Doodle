//
// Created by TD on 2022/5/27.
//

#include "app.h"

#include <doodle_lib/gui/main_menu_bar.h>
#include <doodle_lib/gui/main_status_bar.h>

#include <doodle_lib/core/program_options.h>

#include <gui/window.h>
#include <doodle_lib/doodle_lib_all.h>
using namespace doodle;
void limited_app::load_windows() {
  boost::asio::post(
      make_process_adapter<limited_layout>(strand_gui{g_io_context()})
  );
  boost::asio::post(
      make_process_adapter<main_menu_bar>(strand_gui{g_io_context()})
  );
  boost::asio::post(
      make_process_adapter<main_status_bar>(strand_gui{g_io_context()})
  );
}
