//
// Created by TD on 2022/4/13.
//

#include "layout_window.h"
namespace doodle::gui {
class layout_window::impl {
 public:
  std::vector<warp_w> list_windows;
};
layout_window::layout_window()
    : p_i(std::make_unique<impl>()) {}

const string &layout_window::title() const {
  static std::string l_title{"layout_window"};
  return l_title;
}
void layout_window::init() {
}
void layout_window::succeeded() {
}
void layout_window::update(const chrono::system_clock::duration &in_duration,
                           void *in_data) {
  const ImGuiViewport *viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  dear::Begin("main_windows", &show_,
              ImGuiWindowFlags_NoDecoration |
                  ImGuiWindowFlags_NoMove |
                  ImGuiWindowFlags_NoResize |
                  ImGuiWindowFlags_NoSavedSettings) &&
      []() {

      };
}
layout_window::~layout_window() = default;
}  // namespace doodle::gui
