//
// Created by TD on 2022/1/20.
//

#include "get_input_dialog.h"
#include <lib_warp/imgui_warp.h>
#include <metadata/project.h>
#include <metadata/metadata.h>

namespace doodle {
class get_input_dialog::impl {
 public:
  explicit impl()
      : begin_fun(),
        show(true) {}

  std::vector<std::function<void()>> begin_fun;
  bool show;
};
get_input_dialog::get_input_dialog()
    : p_i(std::make_unique<impl>()) {
}
void get_input_dialog::init() {
  p_i->begin_fun.emplace_back([]() {
    imgui::OpenPopup("get_input_dialog");
    dear::SetNextWindowSize({640, 360});
  });
}
void get_input_dialog::succeeded() {
  imgui::CloseCurrentPopup();
}
void get_input_dialog::failed() {
  imgui::CloseCurrentPopup();
}
void get_input_dialog::aborted() {
  imgui::CloseCurrentPopup();
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

class get_input_project_dialog::impl {
 public:
  project prj;

  std::string path;
};

void get_input_project_dialog::render() {
  dear::Text(fmt::format("路径: {}", p_i->path));
  
  if (dear::InputText("名称", &(p_i->prj.p_name)))
    prj.patch<project>([&](project &in) {
      in.set_name(p_i->prj.p_name);
    });

  if (imgui::Button("ok")) {
    succeed();
  }
}
get_input_project_dialog::get_input_project_dialog(const entt::handle &in_handle)
    : get_input_dialog(),
      prj(in_handle),
      p_i(std::make_unique<impl>()) {
  chick_true<doodle_error>(in_handle.all_of<project>(), DOODLE_LOC, "缺失组件");
}
get_input_project_dialog::~get_input_project_dialog() = default;

void get_input_project_dialog::succeeded() {
  get_input_dialog::succeeded();
}
void get_input_project_dialog::failed() {
  get_input_dialog::failed();
  prj.remove<project>();
}
void get_input_project_dialog::aborted() {
  get_input_dialog::aborted();
  prj.remove<project>();
}
void get_input_project_dialog::init() {
  get_input_dialog::init();
  p_i->prj  = prj.get<project>();
  p_i->path = p_i->prj.p_path.generic_string();
}
}  // namespace doodle
