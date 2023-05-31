//
// Created by td_main on 2023/4/24.
//

#include "solving_fabric_layout.h"

#include "doodle_core/configure/static_value.h"

#include <doodle_lib/gui/layout_window.h>
#include <doodle_lib/gui/menu_bar.h>
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
#include <doodle_lib/long_task/image_to_move.h>

#include <imgui.h>
#include <imgui_internal.h>
namespace doodle::gui {

void solving_fabric_layout::layout(ImGuiID in_id, const ImVec2& in_size) {
  const static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

  ImGui::DockBuilderAddNode(in_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(in_id, in_size);
  /**
   * 分裂给节点 其中 *返回值* 和 out_id_at_dir是相同的, 而另一个是剩下的
   */
  auto dock_id_tools  = in_id;
  auto dock_id_filter = ImGui::DockBuilderSplitNode(in_id, ImGuiDir_Left, 0.2f, nullptr, &dock_id_tools);
  auto dock_id_edit   = ImGui::DockBuilderSplitNode(dock_id_tools, ImGuiDir_Down, 0.5f, nullptr, &dock_id_tools);

  // 开始将窗口停靠在创建的窗口中
  namespace menu_w    = gui::config::menu_w;
  ImGui::DockBuilderDockWindow(menu_w::project_edit.data(), dock_id_filter);   /// \brief 项目配置
  ImGui::DockBuilderDockWindow(menu_w::long_time_tasks.data(), dock_id_edit);  /// \brief maya工具

  ImGui::DockBuilderDockWindow(menu_w::comm_maya_tool.data(), dock_id_tools);  /// \brief 长时间

  ImGui::DockBuilderFinish(in_id);
}

void solving_fabric_layout::set_show() {
  g_windows_manage().open_windows<maya_tool>();
  g_windows_manage().open_windows<long_time_tasks_widget>();
  g_windows_manage().open_windows<project_edit>();

  g_windows_manage().close_windows<edit_widgets>();
  g_windows_manage().close_windows<assets_filter_widget>();
  g_windows_manage().close_windows<assets_file_widgets>();
  g_windows_manage().close_windows<create_video>();
  g_windows_manage().close_windows<extract_subtitles_widgets>();
  g_windows_manage().close_windows<subtitle_processing>();
  g_windows_manage().close_windows<time_sequencer_widget>();
  g_windows_manage().close_windows<xlsx_export_widgets>();
  g_windows_manage().close_windows<setting_windows>();
}

}  // namespace doodle::gui