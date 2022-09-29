//
// Created by TD on 2022/1/18.
//

#include "doodle_main_app.h"

#include <doodle_app/gui/main_status_bar.h>
#include <doodle_lib/gui/menu_bar.h>
#include <doodle_lib/gui/layout_window.h>

#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/get_input_dialog.h>
namespace doodle {
void main_app::load_windows() {
  /// \brief 设置窗口句柄处理
  g_reg()->ctx().at<gui::main_proc_handle>().win_close = []() {
    make_handle().emplace<gui::gui_windows>(std::make_shared<gui::close_exit_dialog>());
  };
  g_reg()->ctx().at<gui::main_proc_handle>().win_destroy = [=]() {
    ::DestroyWindow(p_hwnd);
  };
  make_handle().emplace<gui::gui_tick>() = std::make_shared<gui::layout_window>();
  make_handle().emplace<gui::gui_tick>() = std::make_shared<gui::menu_bar>();
  make_handle().emplace<gui::gui_tick>() = std::make_shared<gui::main_status_bar>();
}
main_app::main_app(const doodle_main_app::in_gui_arg& in_arg)
    : doodle_main_app(in_arg) {
}

}  // namespace doodle
