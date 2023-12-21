//
// Created by TD on 2023/12/21.
//

#include "create_video_layout.h"

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
namespace doodle::gui {

void create_video_layout::set_show() {
  g_windows_manage().open_windows<create_video>();
  g_windows_manage().open_windows<long_time_tasks_widget>();
}
void create_video_layout::layout(ImGuiID in_id, const ImVec2& in_size) {
  const static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

  ImGui::DockBuilderAddNode(in_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(in_id, in_size);
  /**
   * 分裂给节点 其中 *返回值* 和 out_id_at_dir是相同的, 而另一个是剩下的
   */
  auto dock_id_tools = in_id;
  auto dock_id_edit  = ImGui::DockBuilderSplitNode(dock_id_tools, ImGuiDir_Down, 0.5f, nullptr, &dock_id_tools);

  // 开始将窗口停靠在创建的窗口中
  namespace menu_w   = gui::config::menu_w;
  ImGui::DockBuilderDockWindow(menu_w::long_time_tasks.data(), dock_id_edit);  /// \brief maya工具

  ImGui::DockBuilderDockWindow(menu_w::comm_create_video.data(), dock_id_tools);  /// \brief 长时间

  ImGui::DockBuilderFinish(in_id);
}
}  // namespace doodle::gui