//
// Created by TD on 2022/9/29.
//

#include "menu_bar.h"

#include <doodle_core/core/core_sig.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/doodle_core_fwd.h>

#include "doodle_app/gui/base/base_window.h"
#include <doodle_app/app/app_command.h>
#include <doodle_app/gui/base/ref_base.h>
#include <doodle_app/gui/get_input_dialog.h>
#include <doodle_app/gui/open_file_dialog.h>
#include <doodle_app/gui/show_message.h>

#include "doodle_lib/gui/setting_windows.h"
#include <doodle_lib/gui/widgets/all_user_view_widget.h>
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
#include <doodle_lib/toolkit/toolkit.h>

#include "gui/widgets/work_hour_filling.h"
#include <fmt/core.h>
#include <implot.h>

namespace doodle::gui {
void menu_bar::menu_windows() {
  if (dear::MenuItem(setting_windows::name.data())) g_windows_manage().open_windows<setting_windows>();
  if (dear::MenuItem(project_edit::name.data())) g_windows_manage().open_windows<project_edit>();
  if (dear::MenuItem(edit_widgets::name.data())) g_windows_manage().open_windows<edit_widgets>();
  if (dear::MenuItem(assets_filter_widget::name.data())) g_windows_manage().open_windows<assets_filter_widget>();
  if (dear::MenuItem(xlsx_export_widgets::name.data())) g_windows_manage().open_windows<xlsx_export_widgets>();
  if (dear::MenuItem(maya_tool::name.data())) g_windows_manage().open_windows<maya_tool>();
  if (dear::MenuItem(create_video::name.data())) g_windows_manage().open_windows<create_video>();
  if (dear::MenuItem(extract_subtitles_widgets::name.data()))
    g_windows_manage().open_windows<extract_subtitles_widgets>();
  if (dear::MenuItem(subtitle_processing::name.data())) g_windows_manage().open_windows<subtitle_processing>();
  if (dear::MenuItem(assets_file_widgets::name.data())) g_windows_manage().open_windows<assets_file_widgets>();
  if (dear::MenuItem(long_time_tasks_widget::name.data())) g_windows_manage().open_windows<long_time_tasks_widget>();
  if (dear::MenuItem(time_sequencer_widget::name.data())) g_windows_manage().open_windows<time_sequencer_widget>();
  if (dear::MenuItem(all_user_view_widget::name.data())) g_windows_manage().open_windows<all_user_view_widget>();
  //  if (dear::MenuItem(work_hour_filling::name.data())) g_windows_manage().open_windows<work_hour_filling>();
}
void menu_bar::message(const std::string &in_m) {
  g_windows_manage().create_windows_arg(windows_init_arg{}.create<show_message>(in_m).set_title("显示消息"));
}

void menu_bar::menu_tool() {
  if (dear::MenuItem("安装maya插件")) {
    std::string l_message = "安装maya插件{}";
    try {
      toolkit::installMayaPath();
      l_message = fmt::format(l_message, "成功");
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format(l_message, fmt::format("失败{} ", error.what()));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("安装ue4插件")) {
    std::string l_message = "安装ue4插件{}";
    try {
      toolkit::installUePath(core_set::get_set().ue4_path / "Engine");
      l_message = fmt::format(l_message, "成功");
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format(l_message, fmt::format("失败{} ", error.what()));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("安装ue4项目插件")) {
    std::string l_message = "安装ue4项目插件{}";
    try {
      auto *l_file = g_windows_manage().create_windows<file_dialog>(
          file_dialog::dialog_args{}.set_title("选择ue4项目文件").add_filter(".uproject")
      );
      l_file->async_read([](const FSys::path &in) mutable { toolkit::installUePath(in); });
      l_message = fmt::format(l_message, "成功");
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format(l_message, fmt::format("失败{} ", error.what()));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("删除ue4缓存")) {
    std::string l_message = "删除ue4缓存{}";
    try {
      toolkit::deleteUeCache();
      l_message = fmt::format(l_message, "成功");
    } catch (const doodle::doodle_error &error) {
      l_message = fmt::format(l_message, fmt::format("失败{} ", error.what()));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("修改ue4缓存位置")) {
    std::string l_message = "修改ue4缓存位置{}";
    try {
      toolkit::modifyUeCachePath();
      l_message = fmt::format(l_message, "成功");
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format(l_message, fmt::format("失败{} ", error.what()));
    }
    menu_bar::message(l_message);
  }

  if (dear::MenuItem("安装houdini 19.0插件")) {
    std::string l_message = "安装houdini插件{}";
    try {
      toolkit::install_houdini_plug();
      l_message = fmt::format(l_message, "成功");
    } catch (const FSys::filesystem_error &error) {
      l_message = fmt::format(l_message, fmt::format("失败{} ", error.what()));
    }
    menu_bar::message(l_message);
  }
}
}  // namespace doodle::gui
