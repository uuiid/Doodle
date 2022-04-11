//
// Created by TD on 2022/4/8.
//

#include "base_window.h"
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/init_register.h>
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
  auto l_json = core_set::getSet().json_data;
  if (l_json->count(title()))
    (*l_json)[title()].get_to(setting);
  //  core_set_init{}.read_setting(title(), setting);
}
void window_panel::save_setting() const {
  auto l_json        = core_set::getSet().json_data;
  (*l_json)[title()] = setting;
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
const string &window_panel::title() const {
  return title_name_;
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

  dear::PopupModal{title().data(), &show} &&
      [&]() {
        render();
      };
}
void modal_window::close() {
  imgui::CloseCurrentPopup();
}
namespace {
constexpr auto init_base_windows = []() {
  entt::meta<base_window>().type();
};

class init_base_windows_
    : public init_register::registrar_lambda<init_base_windows, 1> {};

constexpr auto init_windows_panel = []() {
  entt::meta<window_panel>().type().base<base_window>();
};

class init_windows_panel_
    : public init_register::registrar_lambda<init_windows_panel, 2> {};

}  // namespace

}  // namespace doodle::gui
