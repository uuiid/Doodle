//
// Created by TD on 2022/1/18.
//

#include "main_facet.h"

#include "doodle_core/gui_template/show_windows.h"
#include <doodle_core/core/program_info.h>

#include "doodle_app/app/authorization.h"
#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_app/gui/main_proc_handle.h>
#include <doodle_app/gui/main_status_bar.h>

#include <doodle_lib/facet/rpc_server_facet.h>
#include <doodle_lib/gui/asset_library_layout.h>
#include <doodle_lib/gui/layout_window.h>
#include <doodle_lib/gui/menu_bar.h>
#include <doodle_lib/gui/setting_windows.h>
#include <doodle_lib/gui/solving_fabric_layout.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/create_video.h>
#include <doodle_lib/gui/widgets/edit_widget.h>
#include <doodle_lib/gui/widgets/extract_subtitles_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/maya_tool.h>
#include <doodle_lib/gui/widgets/project_edit.h>
#include <doodle_lib/gui/widgets/subtitle_processing.h>
#include <doodle_lib/gui/widgets/time_sequencer_widget.h>
#include <doodle_lib/gui/widgets/ue4_widget.h>
#include <doodle_lib/gui/widgets/xlsx_export_widgets.h>
#include <doodle_lib/long_task/image_to_move.h>

#include "boost/asio/ssl.hpp"
#include <boost/asio.hpp>

#include "gui/solving_fabric_layout.h"
namespace doodle {
main_facet::main_facet() : facet::gui_facet() {
  g_reg()->ctx().emplace<image_to_move>() = std::make_shared<detail::image_to_move>();
}

void main_facet::load_windows() {
  {
    using namespace gui;
    g_windows_manage().register_layout(layout_init_arg{}.create<layout_window>());
    g_windows_manage().register_layout(layout_init_arg{}.create<asset_library_layout>());
    g_windows_manage().register_layout(layout_init_arg{}.create<solving_fabric_layout>());
    g_windows_manage().switch_layout(layout_window::name);
  }
  gui::g_windows_manage().create_windows_arg(
      gui::windows_init_arg{}.create<gui::menu_bar>().set_render_type<dear::MainMenuBar>()
  );
  gui::g_windows_manage().create_windows_arg(
      gui::windows_init_arg{}.create<gui::main_status_bar>().set_render_type<dear::ViewportSideBar>(
          nullptr, ImGuiDir_Down
      )
  );
  {
    using namespace gui;
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<edit_widgets>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<assets_filter_widget>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<maya_tool>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<create_video>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<extract_subtitles_widgets>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<subtitle_processing>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<long_time_tasks_widget>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<time_sequencer_widget>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<assets_file_widgets>());
    g_windows_manage().create_windows_arg(windows_init_arg{}.create_set_title<xlsx_export_widgets>());
    g_windows_manage().create_windows_arg(
        windows_init_arg{}.create_set_title<setting_windows>().set_init_hide().set_size(640, 320)
    );
    g_windows_manage().create_windows_arg(
        windows_init_arg{}.create_set_title<project_edit>().set_init_hide().set_size(640, 320)
    );
  }
}

}  // namespace doodle
