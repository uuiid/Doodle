//
// Created by TD on 2022/4/22.
//

#include "maya_layout.h"

#include <maya_plug/configure/static_value.h>
#include <maya_plug/gui/action/comm_check_scenes.h>
#include <maya_plug/gui/action/create_sim_cloth.h>
#include <maya_plug/gui/action/dem_cloth_to_fbx.h>
#include <maya_plug/gui/action/reference_attr_setting.h>

namespace doodle {
namespace maya_plug {

maya_layout::maya_layout() = default;

void maya_layout::layout(ImGuiID in_id, const ImVec2 &in_size) {
  static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
  ImGui::DockBuilderRemoveNode(in_id);  // clear any previous layout
  ImGui::DockBuilderAddNode(in_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);
  ImGui::DockBuilderSetNodeSize(in_id, in_size);

  /**
   * 分裂给节点 其中 *返回值* 和 out_id_at_dir是相同的, 而另一个是剩下的
   */
  auto dock_id_cloth = in_id;
  auto dock_id_chick = ImGui::DockBuilderSplitNode(in_id, ImGuiDir_Left, 0.6f, nullptr, &dock_id_cloth);
  auto dock_id_ref   = ImGui::DockBuilderSplitNode(dock_id_chick, ImGuiDir_Left, 0.5f, nullptr, &dock_id_chick);

  // 开始将窗口停靠在创建的窗口中
  namespace menu_w   = gui::config::maya_plug::menu;
  ImGui::DockBuilderDockWindow(menu_w::comm_check_scenes.data(), dock_id_chick);     /// \brief 过滤器的停靠
  ImGui::DockBuilderDockWindow(menu_w::reference_attr_setting.data(), dock_id_ref);  /// \brief 编辑的停靠
  ImGui::DockBuilderDockWindow(menu_w::dem_cloth_to_fbx.data(), dock_id_ref);        /// \brief 编辑的停靠
  ImGui::DockBuilderDockWindow(menu_w::create_sim_cloth.data(), dock_id_cloth);      /// \brief 编辑的停靠

  ImGui::DockBuilderFinish(in_id);
}

maya_layout::~maya_layout() = default;
void maya_menu::menu_windows() {
  namespace dgui = doodle::gui;
  if (dear::MenuItem(comm_check_scenes::name.data())) gui::g_windows_manage().open_windows<comm_check_scenes>();
  if (dear::MenuItem(reference_attr_setting::name.data()))
    gui::g_windows_manage().open_windows<reference_attr_setting>();
  if (dear::MenuItem(create_sim_cloth::name.data())) gui::g_windows_manage().open_windows<create_sim_cloth>();
  if (dear::MenuItem(dem_cloth_to_fbx::name.data())) gui::g_windows_manage().open_windows<dem_cloth_to_fbx>();
  gui::g_windows_manage().create_windows_arg(gui::windows_init_arg{}.create_set_title<comm_check_scenes>());
  gui::g_windows_manage().create_windows_arg(gui::windows_init_arg{}.create_set_title<reference_attr_setting>());
  gui::g_windows_manage().create_windows_arg(gui::windows_init_arg{}.create_set_title<create_sim_cloth>());
  gui::g_windows_manage().create_windows_arg(gui::windows_init_arg{}.create_set_title<dem_cloth_to_fbx>());
}
void maya_menu::menu_tool() {}
}  // namespace maya_plug
}  // namespace doodle
