//
// Created by TD on 2022/4/13.
//

#include "layout_window.h"

#include "doodle_core/configure/static_value.h"

#include <doodle_app/app/app_command.h>

#include <doodle_lib/gui/setting_windows.h>
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

#include <utility>
namespace doodle::gui {

layout_window::layout_window() = default;

void layout_window::layout(ImGuiID in_id, const ImVec2 &in_size) {
  const ImGuiIO &io                               = ImGui::GetIO();

  const static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

  ImGui::DockBuilderRemoveNode(in_id);  // clear any previous layout
  ImGui::DockBuilderAddNode(in_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(in_id, in_size);

  /**
   * 分裂给节点 其中 *返回值* 和 out_id_at_dir是相同的, 而另一个是剩下的
   */
  auto dock_id_tools  = in_id;
  auto dock_id_filter = ImGui::DockBuilderSplitNode(in_id, ImGuiDir_Left, 0.2f, nullptr, &dock_id_tools);
  auto dock_id_edit   = ImGui::DockBuilderSplitNode(dock_id_filter, ImGuiDir_Down, 0.5f, nullptr, &dock_id_filter);
  auto dock_id_main   = ImGui::DockBuilderSplitNode(dock_id_tools, ImGuiDir_Down, 0.75f, nullptr, &dock_id_tools);

  // 开始将窗口停靠在创建的窗口中
  namespace menu_w    = gui::config::menu_w;
  ImGui::DockBuilderDockWindow(menu_w::assets_filter.data(), dock_id_filter);  /// \brief 过滤器的停靠
  ImGui::DockBuilderDockWindow(menu_w::edit_.data(), dock_id_edit);            /// \brief 编辑的停靠

  ImGui::DockBuilderDockWindow(menu_w::xlsx_export.data(), dock_id_tools);          /// \brief 工具所在的id
  ImGui::DockBuilderDockWindow(menu_w::comm_maya_tool.data(), dock_id_tools);       /// \brief 工具所在的id
  ImGui::DockBuilderDockWindow(menu_w::comm_create_video.data(), dock_id_tools);    /// \brief 工具所在的id
  ImGui::DockBuilderDockWindow(menu_w::extract_subtitles.data(), dock_id_tools);    /// \brief 工具所在的id
  ImGui::DockBuilderDockWindow(menu_w::subtitle_processing.data(), dock_id_tools);  /// \brief 工具所在的id

  ImGui::DockBuilderDockWindow(menu_w::assets_file.data(), dock_id_main);           /// \brief 主窗口的停靠
  ImGui::DockBuilderDockWindow(menu_w::long_time_tasks.data(), dock_id_main);       /// \brief 主窗口的停靠
  ImGui::DockBuilderDockWindow(menu_w::time_edit.data(), dock_id_main);             /// \brief 主窗口的停靠
  ImGui::DockBuilderDockWindow(menu_w::all_user_view_widget.data(), dock_id_main);  /// \brief 主窗口的停靠
  ImGui::DockBuilderDockWindow(menu_w::work_hour_filling.data(), dock_id_main);     /// \brief 主窗口的停靠
  ImGui::DockBuilderFinish(in_id);
}

void layout_window::init_windows() {
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

layout_window::~layout_window() = default;
}  // namespace doodle::gui
