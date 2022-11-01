//
// Created by TD on 2022/1/18.
//

#include "doodle_main_app.h"

#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include <doodle_lib/app/rpc_server_facet.h>
#include <doodle_lib/gui/layout_window.h>
#include <doodle_lib/gui/menu_bar.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <doodle_dingding/client/dingding_api.h>

#include <boost/asio.hpp>

namespace doodle {

main_app::main_app(const doodle_main_app::in_gui_arg& in_arg) : doodle_main_app(in_arg) {
  run_facet = std::make_shared<main_facet>();
  add_facet(run_facet);
  add_facet(std::make_shared<facet::rpc_server_facet>());
}
main_app::main_app() : doodle_main_app() {
  ::ShowWindow(::GetConsoleWindow(), SW_HIDE);
  run_facet = std::make_shared<main_facet>();
  add_facet(run_facet);
  add_facet(std::make_shared<facet::rpc_server_facet>());
}

main_facet::main_facet() : facet::gui_facet() {
  g_reg()->ctx().at<image_to_move>() = std::make_shared<detail::image_to_move>();
}

void main_facet::load_windows() {
  /// \brief 设置窗口句柄处理
  g_reg()->ctx().at<gui::main_proc_handle>().win_close = []() {
    make_handle().emplace<gui::gui_windows>(std::make_shared<gui::close_exit_dialog>());
  };
  g_reg()->ctx().at<gui::main_proc_handle>().win_destroy = [=]() { ::DestroyWindow(p_hwnd); };
  g_reg()->ctx().at<gui::layout_tick>()                  = std::make_shared<gui::layout_window>();
  make_handle().emplace<gui::gui_tick>()                 = std::make_shared<gui::menu_bar>();
  make_handle().emplace<gui::gui_tick>()                 = std::make_shared<gui::main_status_bar>();
}
void main_facet::operator()() {
  gui_facet::operator()();

  /// 初始化上下文
  auto l_ssl = g_reg()->ctx().emplace<std::shared_ptr<boost::asio::ssl::context>>(
      std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::sslv23)
  );
  g_reg()->ctx().emplace<doodle::dingding_api_factory>();
}
void main_facet::deconstruction() {
  g_reg()->ctx().erase<std::shared_ptr<boost::asio::ssl::context>>();
  gui_facet::deconstruction();
}
}  // namespace doodle
