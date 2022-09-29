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

void maya_plug_app::load_windows() {
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_layout>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::main_menu_bar>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::main_status_bar>());
}
void maya_plug_app::close_windows() {
  ::ShowWindow(p_hwnd, SW_HIDE);
}
maya_plug_app::maya_plug_app(const app::in_gui_arg& in_arg)
    : app(in_arg) {
  self = this;
}
void maya_plug_app::post_constructor() {
  app::post_constructor();
  doodle::gui::main_proc_handle::get().win_close   = [this]() { this->close_windows(); };
  doodle::gui::main_proc_handle::get().win_destroy = []() {};
  //  boost::asio::post(g_io_context(), []() {
  //    app::Get().close_windows();
  //  });
}

}  // namespace doodle::maya_plug
