//
// Created by TD on 2022/1/20.
//

#include "get_input_dialog.h"
#include <lib_warp/imgui_warp.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/metadata.h>

#include <utility>

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

  FSys::path path;
  std::string path_gui;
  std::string name;
  std::shared_ptr<FSys::path> in_path;
};

void get_input_project_dialog::render() {
  dear::Text(fmt::format("路径: {}", p_i->path_gui));

  if (dear::InputText("名称", &(p_i->name))) {
    p_i->prj.set_name(p_i->name);
    p_i->path.remove_filename();
    p_i->path /= (p_i->prj.p_name + std::string{doodle_config::doodle_db_name});
    p_i->path_gui = p_i->path.generic_string();
  }

  if (imgui::Button("ok")) {
    succeed();
  }
}
get_input_project_dialog::get_input_project_dialog(const std::shared_ptr<FSys::path> &in_handle)
    : get_input_dialog(),
      p_i(std::make_unique<impl>()) {
  p_i->in_path = in_handle;
}
get_input_project_dialog::~get_input_project_dialog() = default;

void get_input_project_dialog::succeeded() {
  g_reg()->ctx().at<::doodle::database_info>().path_ = p_i->path;
  g_reg()->ctx().at<project>()                       = p_i->prj;
  get_input_dialog::succeeded();
}
void get_input_project_dialog::failed() {
  get_input_dialog::failed();
}
void get_input_project_dialog::aborted() {
  get_input_dialog::aborted();
}
void get_input_project_dialog::init() {
  get_input_dialog::init();
  p_i->path     = *p_i->in_path;
  p_i->path_gui = p_i->path.generic_string();
  p_i->prj.set_path(p_i->path);
}

namespace gui::input {
class get_bool_dialog::impl {
 public:
  explicit impl(std::shared_ptr<bool> is_quit) : quit_(std::move(is_quit)){};
  std::shared_ptr<bool> quit_;
};
void get_bool_dialog::render() {
  ImGui::Text("是否退出?");

  if (ImGui::Button("yes")) {
    *p_i->quit_ = true;
    succeed();
  }
  ImGui::SameLine();
  if (ImGui::Button("no")) {
    *p_i->quit_ = false;
    succeed();
  }
}
get_bool_dialog::get_bool_dialog(const std::shared_ptr<bool> &is_quit)
    : p_i(std::make_unique<impl>(is_quit)) {
}

}  // namespace gui::input
}  // namespace doodle
