//
// Created by TD on 2022/1/20.
//

#include "get_input_dialog.h"
#include <lib_warp/imgui_warp.h>
#include <metadata/project.h>

namespace doodle {
class get_input_dialog::impl {
 public:
  explicit impl()
      : begin_fun(),
        show(false) {}

  std::vector<std::function<void()>> begin_fun;
  bool show;
};
get_input_dialog::get_input_dialog()
    : p_i(std::make_unique<impl>()) {
}
void get_input_dialog::init() {
  p_i->begin_fun.emplace_back([]() {
    imgui::OpenPopup("get_input_dialog");
    dear::SetWindowSize({640, 360});
  });
}
void get_input_dialog::succeeded() {
}
void get_input_dialog::failed() {
}
void get_input_dialog::aborted() {
}
void get_input_dialog::update(chrono::duration<chrono::system_clock::rep, chrono::system_clock::period>, void *data) {
  for (auto &&i : p_i->begin_fun) {
    i();
  }
  p_i->begin_fun.clear();

  dear::PopupModal{"get_input_dialog", &p_i->show} && [this]() {
    this->render();
  };
}
get_input_dialog::~get_input_dialog() = default;
void get_input_project_dialog::render() {
  gui::render<project>(prj);

  if (imgui::Button("ok")) {
    succeed();
  }
}
get_input_project_dialog::get_input_project_dialog(const entt::handle &in_handle)
    : get_input_dialog(),
      prj(in_handle) {
}
}  // namespace doodle
