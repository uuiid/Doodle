//
// Created by TD on 2022/4/13.
//

#include "layout_window.h"
namespace doodle::gui {
class layout_window::impl {
 public:
  std::map<std::string, warp_w> list_windows;
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
      [&, this]() {
        std::string prj{gui::config::menu_w::project_widget};
        dear::Child{gui::config::menu_w::project_widget.data(),
                    p_i->list_windows[prj].windows_->size()} &&
            [&, this]() {
              p_i->list_windows[prj]
                  .windows_->update(in_duration,
                                    in_data);
            };
      };
}
layout_window::~layout_window() = default;
}  // namespace doodle::gui
