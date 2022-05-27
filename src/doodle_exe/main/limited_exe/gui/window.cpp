//
// Created by TD on 2022/5/27.
//

#include "window.h"
void limited_layout::update(const chrono::system_clock::duration &in_duration, void *in_data) {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);

  dear::Begin{"main_windows", &show_,
              ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoSavedSettings} &&
      [&, this]() {
        namespace menu_w = gui::config::menu_w;
        dear::Child{"l1", ImVec2{viewport->WorkSize.x / 4, 0}, false} && [&, this]() {
          dear::Child{"ll1", ImVec2{0, viewport->WorkSize.y / 6}} && [&]() { call_render(std::string{menu_w::project_widget}); };
          dear::Child{"ll2", ImVec2{0, viewport->WorkSize.y / 3}} && [&]() { call_render(std::string{menu_w::assets_filter}); };
          dear::Child{"ll3"} && [&]() { call_render(std::string{menu_w::edit_}); };
        };
        ImGui::SameLine();
        dear::Child{"l4"} && [&]() {
          dear::Child{"l2", ImVec2{0, viewport->WorkSize.y / 4}, true} && [&, this]() {
            dear::TabBar{"##tool",
                         ImGuiTabBarFlags_Reorderable |
                             ImGuiTabBarFlags_FittingPolicyResizeDown} &&
                [&, this]() {
                  dear::TabItem{menu_w::comm_maya_tool.data()} && [&]() { call_render(std::string{menu_w::comm_maya_tool}); };
                };
          };
          //        ImGui::SameLine();
          dear::Child{"l3", ImVec2{0, 0}, true} && [&, this]() {
            dear::TabBar{"##main"} && [&]() {
              dear::TabItem{menu_w::assets_file.data()} && [&]() { main_render(); };
              dear::TabItem{menu_w::long_time_tasks.data()} && [&]() { call_render(std::string{menu_w::long_time_tasks}); };
            };
          };
        };
      };
  clear_windows();
}
