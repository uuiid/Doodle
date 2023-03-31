//
// Created by td_main on 2023/3/28.
//

#include "layout_window_base.h"

#include <imgui.h>
namespace doodle::gui::details {

bool layout_window_base::render() {
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
  const ImGuiIO &io = ImGui::GetIO();
  if (!(io.ConfigFlags & ImGuiConfigFlags_DockingEnable)) return true;
  ImGui::Begin(name().c_str(), nullptr, window_flags);
  const static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
  const ImGuiID dockspace_id                      = ImGui::GetID("DOODLE_DockSpace_Root");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

  std::call_once(once_flag_, [this, dockspace_id, viewport]() { this->layout(dockspace_id, viewport->Size); });
  ImGui::End();
  ImGui::PopStyleVar(3);

  return true;
}
std::string &layout_window_base::name() { return name_; }
}  // namespace doodle::gui::details