//
// Created by TD on 2022/9/29.
//

#include "menu_bar.h"

#include <doodle_app/app/app_command.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/gui/setting_windows.h>
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_app/gui/base/ref_base.h>

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/database_task/sqlite_client.h>

#include <doodle_lib/toolkit/toolkit.h>

#include <doodle_lib/gui/widgets/project_edit.h>
#include <doodle_lib/gui/widgets/edit_widget.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/csv_export_widgets.h>
#include <doodle_lib/gui/widgets/maya_tool.h>
#include <doodle_lib/gui/widgets/create_video.h>
#include <doodle_lib/gui/widgets/ue4_widget.h>
#include <doodle_lib/gui/widgets/extract_subtitles_widgets.h>
#include <doodle_lib/gui/widgets/subtitle_processing.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/time_sequencer_widget.h>
#include <doodle_lib/gui/widgets/all_user_view_widget.h>


namespace doodle::gui {
void menu_bar::menu_windows() {
  if (dear::MenuItem(setting_windows::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<setting_windows>());
  if (dear::MenuItem(project_edit::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<project_edit>());
  if (dear::MenuItem(edit_widgets::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<edit_widgets>());
  if (dear::MenuItem(assets_filter_widget::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<assets_filter_widget>());
  if (dear::MenuItem(csv_export_widgets::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<csv_export_widgets>());
  if (dear::MenuItem(maya_tool::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<maya_tool>());
  if (dear::MenuItem(create_video::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<create_video>());
  if (dear::MenuItem(extract_subtitles_widgets::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<extract_subtitles_widgets>());
  if (dear::MenuItem(subtitle_processing::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<subtitle_processing>());
  if (dear::MenuItem(assets_file_widgets::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<assets_file_widgets>());
  if (dear::MenuItem(long_time_tasks_widget::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<long_time_tasks_widget>());
  if (dear::MenuItem(time_sequencer_widget::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<time_sequencer_widget>());
  if (dear::MenuItem(all_user_view_widget::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<all_user_view_widget>());
  if (dear::MenuItem(ue4_widget::name.data()))
    make_handle().emplace<gui_windows>(std::make_shared<ue4_widget>());
}
void menu_bar::menu_tool() {
  if (dear::MenuItem("安装maya插件"))
    toolkit::installMayaPath();
  if (dear::MenuItem("安装ue4插件"))
    toolkit::installUePath(core_set::get_set().ue4_path / "Engine");
  if (dear::MenuItem("安装ue4项目插件")) {
    auto l_file = std::make_shared<file_dialog>(
        file_dialog::dialog_args{}
            .set_title("选择ue4项目文件")
            .add_filter(".uproject")
    );
    auto l_f_h = make_handle();
    l_f_h.emplace<gui_windows>(l_file);
    l_file->async_read([l_f_h](const FSys::path &in) mutable {
      toolkit::installUePath(in);
      l_f_h.destroy();
    });
  }
  if (dear::MenuItem("删除ue4缓存"))
    toolkit::deleteUeCache();
  if (dear::MenuItem("修改ue4缓存位置"))
    toolkit::modifyUeCachePath();
  if (dear::MenuItem("安装houdini 19.0插件"))
    toolkit::install_houdini_plug();
}
}  // namespace doodle::gui
