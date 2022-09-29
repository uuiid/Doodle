//
// Created by TD on 2022/1/18.
//

#include "doodle_main_app.h"

#include <doodle_app/gui/main_status_bar.h>
#include <doodle_lib/gui/menu_bar.h>
#include <doodle_lib/gui/layout_window.h>
void doodle::main_app::load_windows() {
  make_handle().emplace<gui::gui_tick>() = std::make_shared<gui::layout_window>();
  make_handle().emplace<gui::gui_tick>() = std::make_shared<gui::menu_bar>();
  make_handle().emplace<gui::gui_tick>() = std::make_shared<gui::main_status_bar>();
}
