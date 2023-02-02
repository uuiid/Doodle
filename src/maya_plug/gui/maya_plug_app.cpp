//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/core/core_set.h>

#include <doodle_app/gui/main_menu_bar.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include <boost/asio/post.hpp>

#include <maya_plug/data/maya_create_movie.h>
#include <maya_plug/data/null_facet.h>
#include <maya_plug/gui/maya_layout.h>

#include <maya/MGlobal.h>

namespace doodle::maya_plug {
void maya_facet::load_windows() {
  gui::main_proc_handle::value().win_close   = [this]() { this->close_windows(); };
  gui::main_proc_handle::value().win_destroy = []() {};
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_layout>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<maya_menu>());
  make_handle().emplace<gui::gui_tick>(std::make_shared<gui::main_status_bar>());
  boost::asio::post(g_io_context(), [this]() { close_windows(); });
}
void maya_facet::close_windows() { ::ShowWindow(p_hwnd, SW_HIDE); }
maya_facet::maya_facet() : doodle::facet::gui_facet() {
  g_reg()->ctx().at<image_to_move>() = std::make_shared<detail::maya_create_movie>();
}

}  // namespace doodle::maya_plug
