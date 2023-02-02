//
// Created by TD on 2022/1/18.
//

#include "main_facet.h"

#include <doodle_core/core/program_info.h>

#include "doodle_app/app/authorization.h"
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include <doodle_lib/facet/rpc_server_facet.h>
#include <doodle_lib/gui/layout_window.h>
#include <doodle_lib/gui/menu_bar.h>
#include <doodle_lib/long_task/image_to_move.h>

#include "boost/asio/ssl.hpp"
#include <boost/asio.hpp>

namespace doodle {
main_facet::main_facet() : facet::gui_facet() {
  g_reg()->ctx().at<image_to_move>() = std::make_shared<detail::image_to_move>();
}

void main_facet::load_windows() {
  ::ShowWindow(::GetConsoleWindow(), SW_HIDE);

  /// \brief 设置窗口句柄处理
  gui::main_proc_handle::value().win_close = [this]() {
    if (::GetForegroundWindow() == p_hwnd)
      make_handle().emplace<gui::gui_windows>(std::make_shared<gui::close_exit_dialog>());
    else
      close_windows();
  };
  gui::main_proc_handle::value().win_destroy = [=]() { ::DestroyWindow(p_hwnd); };
  g_reg()->ctx().at<gui::layout_tick>()      = std::make_shared<gui::layout_window>();
  make_handle().emplace<gui::gui_tick>()     = std::make_shared<gui::menu_bar>();
  make_handle().emplace<gui::gui_tick>()     = std::make_shared<gui::main_status_bar>();
}

}  // namespace doodle
