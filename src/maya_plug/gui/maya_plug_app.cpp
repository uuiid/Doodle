//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include <doodle_app/gui/main_status_bar.h>
#include <doodle_app/gui/main_menu_bar.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_core/core/core_set.h>
#include <maya_plug/gui/maya_layout.h>
#include <doodle_app/gui/main_proc_handle.h>
namespace doodle::maya_plug {



void maya_facet::load_windows() {
  g_reg()->ctx().at<gui::main_proc_handle>().win_close   = [this]() { this->close_windows(); };
  g_reg()->ctx().at<gui::main_proc_handle>().win_destroy = []() {};
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_layout>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::main_menu_bar>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::main_status_bar>());
}
void maya_facet::close_windows() {
  ::ShowWindow(p_hwnd, SW_HIDE);
}
}  // namespace doodle::maya_plug
