//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/thread_pool/process_pool.h>

#include <doodle_app/gui/main_menu_bar.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include <maya_plug/data/maya_create_movie.h>
#include <maya_plug/data/null_facet.h>
#include <maya_plug/gui/maya_layout.h>

#include <maya/MGlobal.h>

namespace doodle::maya_plug {
void maya_facet::load_windows() {
  g_reg()->ctx().at<gui::main_proc_handle>().win_close   = [this]() { this->close_windows(); };
  g_reg()->ctx().at<gui::main_proc_handle>().win_destroy = []() {};
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_layout>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_menu>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::main_status_bar>());
}
void maya_facet::close_windows() {
  ::ShowWindow(p_hwnd, SW_HIDE);
}
maya_facet::maya_facet() : doodle::facet::gui_facet() {
  g_reg()->ctx().at<image_to_move>() = std::make_shared<detail::maya_create_movie>();
}

maya_plug_app::maya_plug_app() {
  set_facet();
}
maya_plug_app::maya_plug_app(
    const doodle_main_app::in_gui_arg& inArg
) : doodle_main_app(inArg) {
  set_facet();
}
void maya_plug_app::set_facet() {
  switch (MGlobal::mayaState()) {
    case MGlobal::MMayaState::kBaseUIMode:
    case MGlobal::MMayaState::kInteractive: {
      run_facet = std::make_shared<maya_facet>();
      break;
    }
    case MGlobal::MMayaState::kBatch:
    case MGlobal::MMayaState::kLibraryApp:
    default: {
      run_facet = std::make_shared<null_facet>();
    } break;
  }
  add_facet(run_facet);
}
}  // namespace doodle::maya_plug
