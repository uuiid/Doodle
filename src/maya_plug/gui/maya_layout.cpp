//
// Created by TD on 2022/4/22.
//

#include "maya_layout.h"

#include "doodle_lib/gui/setting_windows.h"

#include <maya_plug/configure/static_value.h>
#include <maya_plug/gui/action/comm_check_scenes.h>
#include <maya_plug/gui/action/create_sim_cloth.h>
#include <maya_plug/gui/action/dem_cloth_to_fbx.h>
#include <maya_plug/gui/action/reference_attr_setting.h>

namespace doodle {
namespace maya_plug {

class maya_layout::impl {
 public:
  bool inited{false};

  void builder_dock() {
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

    window_flags |= ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoCollapse |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    // 如果使用 ImGuiDockNodeFlags_PassthruCentralNode 处理, 那么我们就不使用背景
    window_flags |= ImGuiWindowFlags_NoBackground;

    /**
     * 这里我们持续的使 对接窗口在活动的状态上, 如果 对接处于非活动状态, 那么所有的活动窗口
     * 都会丢失父窗口并脱离, 我们将无法保留停靠窗口和非停靠窗口之间的关系, 这将导致窗口被困在边缘,
     * 永远的不可见
     */
    ImGui::Begin(
        "Doodle_DockSpace",
        nullptr,
        window_flags
    );
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
        auto dock_id_cloth = dockspace_id;
        auto dock_id_chick = ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.6f, nullptr, &dock_id_cloth);
        auto dock_id_ref   = ImGui::DockBuilderSplitNode(dock_id_chick, ImGuiDir_Left, 0.5f, nullptr, &dock_id_chick);

        // 开始将窗口停靠在创建的窗口中
        namespace menu_w   = gui::config::maya_plug::menu;
        ImGui::DockBuilderDockWindow(menu_w::comm_check_scenes.data(), dock_id_chick);     /// \brief 过滤器的停靠
        ImGui::DockBuilderDockWindow(menu_w::reference_attr_setting.data(), dock_id_ref);  /// \brief 编辑的停靠
        ImGui::DockBuilderDockWindow(menu_w::dem_cloth_to_fbx.data(), dock_id_ref);        /// \brief 编辑的停靠
        ImGui::DockBuilderDockWindow(menu_w::create_sim_cloth.data(), dock_id_cloth);      /// \brief 编辑的停靠

        ImGui::DockBuilderFinish(dockspace_id);
      }
    }
    ImGui::End();
  }
};
maya_layout::maya_layout()
    : p_i(std::make_unique<impl>()) {
}
bool maya_layout::tick() {
  p_i->builder_dock();
  if (!p_i->inited) {
    p_i->inited = true;
    make_handle().emplace<gui::gui_windows>(std::make_shared<comm_check_scenes>());
    make_handle().emplace<gui::gui_windows>(std::make_shared<reference_attr_setting>());
    make_handle().emplace<gui::gui_windows>(std::make_shared<create_sim_cloth>());
    make_handle().emplace<gui::gui_windows>(std::make_shared<dem_cloth_to_fbx>());
  }
  return false ;
}

maya_layout::~maya_layout() = default;
void maya_menu::menu_windows() {
  namespace dgui = doodle::gui;
  if (dear::MenuItem(dgui::setting_windows::name.data()))
    make_handle().emplace<dgui::gui_windows>(std::make_shared<dgui::setting_windows>());
  if (dear::MenuItem(comm_check_scenes::name.data()))
    make_handle().emplace<dgui::gui_windows>(std::make_shared<comm_check_scenes>());
  if (dear::MenuItem(reference_attr_setting::name.data()))
    make_handle().emplace<dgui::gui_windows>(std::make_shared<reference_attr_setting>());
  if (dear::MenuItem(create_sim_cloth::name.data()))
    make_handle().emplace<dgui::gui_windows>(std::make_shared<create_sim_cloth>());
  if (dear::MenuItem(dem_cloth_to_fbx::name.data()))
    make_handle().emplace<dgui::gui_windows>(std::make_shared<dem_cloth_to_fbx>());
}
void maya_menu::menu_tool() {
}
}  // namespace maya_plug
}  // namespace doodle
