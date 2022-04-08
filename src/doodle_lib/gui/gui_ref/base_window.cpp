//
// Created by TD on 2022/4/8.
//

#include "base_window.h"
#include <doodle_lib/core/core_set.h>

namespace doodle::gui {

void base_window::failed() {}
void base_window::update(const chrono::system_clock::duration &in_duration, void *in_data) {
  for (auto &&i : begin_fun) {
    i();
  }
  begin_fun.clear();

  dear::Begin{title().c_str(), &show} &&
      [&]() {
        this->render();
      };
}

void window_panel::read_setting() {
  (*core_set::getSet().json_data)[title()].get_to(setting);
  //  core_set_init{}.read_setting(title(), setting);
}
void window_panel::save_setting() const {
  (*core_set::getSet().json_data)[title()] = setting;
  //  core_set_init{}.save_setting(title(), setting);
}
void window_panel::init() {
  read_setting();
}
void window_panel::succeeded() {
  save_setting();
}

void window_panel::aborted() {
  save_setting();
}

modal_window::modal_window()
    : show{true} {
  begin_fun.emplace_back([this]() {
    ImGui::OpenPopup(title().data());
    ImGui::SetNextWindowSize({640, 360});
  });
}
void modal_window::update(const chrono::system_clock::duration &in_dalta, void *in_data) {
  for (auto &&i : begin_fun) {
    i();
  }
  begin_fun.clear();
  //    if (!show)
  //      This()->fail();

  dear::PopupModal{title().data(), &show} &&
      [&]() {
        render();
      };
}
void modal_window::close() {
  {
    imgui::CloseCurrentPopup();
  }
}

}  // namespace doodle::gui
