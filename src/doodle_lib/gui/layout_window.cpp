//
// Created by TD on 2022/4/13.
//

#include "layout_window.h"
#include "doodle_core/configure/static_value.h"

#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/create_video.h>
#include <doodle_lib/gui/widgets/xlsx_export_widgets.h>
#include <doodle_lib/gui/widgets/edit_widget.h>
#include <doodle_lib/gui/widgets/extract_subtitles_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/maya_tool.h>
#include <doodle_lib/gui/widgets/subtitle_processing.h>
#include <doodle_lib/gui/widgets/time_sequencer_widget.h>
#include <doodle_lib/gui/widgets/ue4_widget.h>

#include <utility>
namespace doodle::gui {
class layout_window::impl {
 public:
  impl() = default;

  bool init{false};

  static void builder_dock() {
    // 我们使用ImGuiWindowFlags_NoDocking标志来使窗口不可停靠到父窗口中，因为在彼此之间有两个停靠目标会令人困惑
    ImGuiWindowFlags window_flags =  // 没有菜单 ImGuiWindowFlags_MenuBar |
        ImGuiWindowFlags_NoDocking;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // 如果使用 ImGuiDockNodeFlags_PassthruCentralNode 处理, 那么我们就不使用背景
    window_flags |= ImGuiWindowFlags_NoBackground;

    /**
     * 这里我们持续的使 对接窗口在活动的状态上, 如果 对接处于非活动状态, 那么所有的活动窗口
     * 都会丢失父窗口并脱离, 我们将无法保留停靠窗口和非停靠窗口之间的关系, 这将导致窗口被困在边缘,
     * 永远的不可见
     */
    ImGui::Begin("Doodle_DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
    ImGuiIO &io                               = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
      ImGuiID dockspace_id = ImGui::GetID("DOODLE_DockSpace_Root");
      ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

      static auto first_time = true;
      if (first_time) {
        first_time = false;

        ImGui::DockBuilderRemoveNode(dockspace_id);  // clear any previous layout
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

        /**
         * 分裂给节点 其中 *返回值* 和 out_id_at_dir是相同的, 而另一个是剩下的
         */
        auto dock_id_tools  = dockspace_id;
        auto dock_id_filter = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.2f, nullptr, &dock_id_tools);
        auto dock_id_edit = ImGui::DockBuilderSplitNode(dock_id_filter, ImGuiDir_Down, 0.5f, nullptr, &dock_id_filter);
        auto dock_id_main = ImGui::DockBuilderSplitNode(dock_id_tools, ImGuiDir_Down, 0.75f, nullptr, &dock_id_tools);

        // 开始将窗口停靠在创建的窗口中
        namespace menu_w  = gui::config::menu_w;
        ImGui::DockBuilderDockWindow(menu_w::assets_filter.data(), dock_id_filter);  /// \brief 过滤器的停靠
        ImGui::DockBuilderDockWindow(menu_w::edit_.data(), dock_id_edit);            /// \brief 编辑的停靠

        ImGui::DockBuilderDockWindow(menu_w::xlsx_export.data(), dock_id_tools);           /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::ue4_widget.data(), dock_id_tools);           /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::comm_maya_tool.data(), dock_id_tools);       /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::comm_create_video.data(), dock_id_tools);    /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::extract_subtitles.data(), dock_id_tools);    /// \brief 工具所在的id
        ImGui::DockBuilderDockWindow(menu_w::subtitle_processing.data(), dock_id_tools);  /// \brief 工具所在的id

        ImGui::DockBuilderDockWindow(menu_w::assets_file.data(), dock_id_main);           /// \brief 主窗口的停靠
        ImGui::DockBuilderDockWindow(menu_w::long_time_tasks.data(), dock_id_main);       /// \brief 主窗口的停靠
        ImGui::DockBuilderDockWindow(menu_w::time_edit.data(), dock_id_main);             /// \brief 主窗口的停靠
        ImGui::DockBuilderDockWindow(menu_w::all_user_view_widget.data(), dock_id_main);  /// \brief 主窗口的停靠
        ImGui::DockBuilderDockWindow(menu_w::work_hour_filling.data(), dock_id_main);  /// \brief 主窗口的停靠
        ImGui::DockBuilderFinish(dockspace_id);
      }
    }
    ImGui::End();
  }
};
layout_window::layout_window() : p_i(std::make_unique<impl>()) {}

// const std::string &layout_window::title() const {
//   static std::string l_title{"layout_window"};
//   return l_title;
// }

bool layout_window::tick() {
  p_i->builder_dock();
  if (!p_i->init) {  /// 初始化窗口
    p_i->init = true;
    show_windows<edit_widgets>();
    show_windows<assets_filter_widget>();
    show_windows<maya_tool>();
    show_windows<create_video>();
    show_windows<extract_subtitles_widgets>();
    show_windows<subtitle_processing>();
    show_windows<long_time_tasks_widget>();
    show_windows<time_sequencer_widget>();
    show_windows<ue4_widget>();
    show_windows<assets_file_widgets>();
    show_windows<xlsx_export_widgets>();
  }

  return false;
}

// template <typename windows_type>
// void layout_window::call_render() {
//   auto &&l_win = p_i->list_windows[""s];
//   if (!l_win) {
//   }
// }

layout_window::~layout_window() = default;
}  // namespace doodle::gui
