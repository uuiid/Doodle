//
// Created by TD on 2022/4/22.
//

#include "maya_layout.h"
#include <maya_plug/configure/static_value.h>

namespace doodle {
namespace maya_plug {
void maya_layout::update(const chrono::system_clock::duration &in_duration, void *in_data) {
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
        dear::Child{"l2", ImVec2{viewport->WorkSize.x / 3, 0}, true} && [&, this]() {
          main_render();
        };
        ImGui::SameLine();
        dear::Child{"l3", ImVec2{0, 0}, true} && [&, this]() {
          dear::Child{"l33", ImVec2{0, viewport->WorkSize.y / 3}, false} && [&, this]() {
            dear::TabBar{"##tool",
                         ImGuiTabBarFlags_Reorderable |
                             ImGuiTabBarFlags_FittingPolicyResizeDown} &&
                [&, this]() {
                  namespace maya_menu = gui::config::maya_plug::menu;
                  dear::TabItem{maya_menu::comm_check_scenes.data()} && [&]() { call_render(std::string{maya_menu::comm_check_scenes}); };
                  dear::TabItem{maya_menu::create_sim_cloth.data()} && [&]() { call_render(std::string{maya_menu::create_sim_cloth}); };
                  dear::TabItem{maya_menu::reference_attr_setting.data()} && [&]() { call_render(std::string{maya_menu::reference_attr_setting}); };
                };
          };
          call_render(std::string{menu_w::long_time_tasks});
        };
      };
  clear_windows();
}
}  // namespace maya_plug
}  // namespace doodle
