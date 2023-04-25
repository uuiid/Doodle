//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/gui_template/show_windows.h"
#include <doodle_core/core/core_set.h>

#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/gui/main_menu_bar.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include <boost/asio/post.hpp>

#include <maya_plug/data/maya_create_movie.h>
#include <maya_plug/data/null_facet.h>
#include <maya_plug/gui/action/comm_check_scenes.h>
#include <maya_plug/gui/action/create_sim_cloth.h>
#include <maya_plug/gui/action/dem_cloth_to_fbx.h>
#include <maya_plug/gui/action/reference_attr_setting.h>
#include <maya_plug/gui/maya_layout.h>

#include <maya/MGlobal.h>

namespace doodle::maya_plug {
void maya_facet::load_windows() {
  using namespace gui;

  g_windows_manage().register_layout(layout_init_arg{}.create<maya_layout>());
  g_windows_manage().switch_layout(maya_layout::name);

  g_windows_manage().create_windows_arg(windows_init_arg{}.create<maya_menu>().set_render_type<dear::MainMenuBar>());
  g_windows_manage().create_windows_arg(
      windows_init_arg{}.create<main_status_bar>().set_render_type<dear::ViewportSideBar>(nullptr, ImGuiDir_Down)
  );
  g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<comm_check_scenes>());
  g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<reference_attr_setting>());
  g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<create_sim_cloth>());
  g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<dem_cloth_to_fbx>());

  boost::asio::post(g_io_context(), [this]() { close_windows(); });
}
void maya_facet::close_windows() { ::ShowWindow(p_hwnd, SW_HIDE); }
maya_facet::maya_facet() : doodle::facet::gui_facet() {
  g_reg()->ctx().get<image_to_move>() = std::make_shared<detail::maya_create_movie>();
}

}  // namespace doodle::maya_plug
