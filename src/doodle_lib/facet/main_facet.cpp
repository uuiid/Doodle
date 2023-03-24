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
  this->set_layout(gui::windows{std::in_place_type<gui::layout_window>});
  gui::g_windows_manage().create_windows<gui::menu_bar>();
  gui::g_windows_manage().create_windows<gui::main_status_bar>();
}

}  // namespace doodle
