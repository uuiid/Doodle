//
// Created by TD on 2022/1/20.
//

#include "get_input_dialog.h"
#include <lib_warp/imgui_warp.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/metadata.h>

#include <utility>

#include <doodle_lib/gui/gui_ref/ref_base.h>
#include <doodle_lib/gui/open_file_dialog.h>

namespace doodle::gui {

class create_project_dialog::impl {
 public:
  project prj;

  FSys::path path;
  std::string path_gui;
  std::string name;
  std::shared_ptr<FSys::path> in_path;
  gui_cache<entt::handle> select_button_id{"选择文件夹", make_handle()};

  std::string title{"输入项目"s};
};

void create_project_dialog::render() {
  dear::Text(fmt::format("路径: {}", p_i->path_gui));

  if (ImGui::Button(*p_i->select_button_id)) {
    file_dialog l_file{file_dialog::dialog_args{}.set_use_dir()};
    l_file.async_read([this](const FSys::path& in) {
      p_i->path     = in / (p_i->prj.p_name + std::string{doodle_config::doodle_db_name});
      p_i->path_gui = p_i->path.generic_string();
    });
    p_i->select_button_id().emplace_or_replace<gui_windows>(l_file);
  }

  if (dear::InputText("名称", &(p_i->name))) {
    p_i->prj.set_name(p_i->name);
    p_i->path.remove_filename();
    p_i->path /= (p_i->prj.p_name + std::string{doodle_config::doodle_db_name});
    p_i->path_gui = p_i->path.generic_string();
  }

  if (imgui::Button("ok")) {
    ImGui::CloseCurrentPopup();
    p_i->select_button_id().destroy();
    g_reg()->ctx().at<::doodle::database_info>().path_ = p_i->path;
    g_reg()->ctx().at<project>()                       = p_i->prj;
  }
}

create_project_dialog::create_project_dialog()
    : p_i(std::make_unique<impl>()) {
  p_i->path     = core_set::get_set().get_doc() / "doodle";
  p_i->path_gui = p_i->path.generic_string();
}
create_project_dialog::~create_project_dialog() = default;

const std::string& create_project_dialog::title() const {
  return p_i->title;
}
void create_project_dialog::set_attr() const {
  ImGui::OpenPopup(title().data());
  ImGui::SetNextWindowSize({640, 360});
}
std::int32_t create_project_dialog::flags() const {
  boost::ignore_unused(this);
  return ImGuiWindowFlags_NoSavedSettings;
}

class close_exit_dialog::impl {
 public:
  explicit impl(std::shared_ptr<bool> is_quit) : quit_(std::move(is_quit)){};
  std::shared_ptr<bool> quit_;
  std::string title{"退出"s};
};
void close_exit_dialog::render() {
  ImGui::Text("是否退出?");

  if (ImGui::Button("yes")) {
    *p_i->quit_ = true;
    ImGui::CloseCurrentPopup();
  }
  ImGui::SameLine();
  if (ImGui::Button("no")) {
    *p_i->quit_ = false;
    ImGui::CloseCurrentPopup();
  }
}
void close_exit_dialog::set_attr() const {
  ImGui::OpenPopup(title().data());
  ImGui::SetNextWindowSize({640, 360});
}
close_exit_dialog::close_exit_dialog(std::shared_ptr<bool> is_quit)
    : p_i(std::make_unique<impl>(std::move(is_quit))) {
}
const std::string& close_exit_dialog::title() const {
  return p_i->title;
}
std::int32_t close_exit_dialog::flags() const {
  boost::ignore_unused(this);
  return ImGuiWindowFlags_NoSavedSettings;
}
}  // namespace doodle::gui
