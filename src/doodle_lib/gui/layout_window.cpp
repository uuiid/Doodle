//
// Created by TD on 2022/4/13.
//

#include "layout_window.h"

#include "doodle_core/configure/static_value.h"

#include <doodle_app/app/app_command.h>

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

std::string &layout_window::name() { return name_; }

layout_window::~layout_window() = default;
}  // namespace doodle::gui
