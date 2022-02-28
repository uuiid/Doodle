//
// Created by TD on 2022/1/20.
//

#include "project.h"

#include <lib_warp/imgui_warp.h>
#include <metadata/project.h>

#include <doodle_lib/lib_warp/imgui_warp.h>

namespace doodle::gui {
class project_edit::impl {
 public:
  gui_cache<std::string> project_path;
  gui_cache<std::string> project_name;
  impl()
      : project_path("路径"s, ""s),
        project_name("名称"s, ""s) {}
};

project_edit::project_edit()
    : p_i(std::make_unique<impl>()) {
}

void project_edit::init_(const entt::handle& in) {
  auto& l_prj       = in.get<project>();
  p_i->project_path = l_prj.get_path().generic_string();
  p_i->project_name = l_prj.p_name;
}
void project_edit::save_(const entt::handle& in) const {
  in.emplace_or_replace<project>(p_i->project_path.data, p_i->project_name.data);
  g_reg()->set<project>(in.get<project>());
}
void project_edit::render(const entt::handle& in) {
  ImGui::Text("基本配置");
  dear::Text(fmt::format("{}: {}", p_i->project_path.gui_name.name, p_i->project_path.data));

  if (ImGui::InputText(*p_i->project_name.gui_name, &p_i->project_name.data)) {
    set_modify(true);
  }
}

project_edit::~project_edit() = default;

class cloth_config_edit::impl {
 public:
  impl()
      : path_("解算路径"s, ""s),
        ue4_name("导出组名称"s, ""s) {}
  gui_cache<std::string> path_;
  gui_cache<std::string> ue4_name;
};

void cloth_config_edit::init_(const entt::handle& in) {
  auto& l_c     = in.get_or_emplace<project::cloth_config>();
  p_i->path_    = l_c.vfx_cloth_sim_path.generic_string();
  p_i->ue4_name = l_c.export_group;
}
void cloth_config_edit::save_(const entt::handle& in) const {
  auto& l_c              = in.get_or_emplace<project::cloth_config>();
  l_c.vfx_cloth_sim_path = p_i->path_.data;
  l_c.export_group       = p_i->ue4_name;
}
cloth_config_edit::cloth_config_edit()
    : p_i(std::make_unique<impl>()) {
}
void cloth_config_edit::render(const entt::handle& in) {
  if (imgui::InputText(*p_i->path_.gui_name, &(p_i->path_.data))) {
    set_modify(true);
  }
  if (imgui::InputText(*p_i->ue4_name.gui_name, &(p_i->ue4_name.data))) {
    set_modify(true);
  };
}
cloth_config_edit::~cloth_config_edit() = default;

class modle_config_edit::impl {
 public:
  impl() : regex_("正则表达式"s, ""s) {}
  gui_cache<std::string> regex_;
  std::string err_str;
};

modle_config_edit::modle_config_edit()
    : p_i(std::make_unique<impl>()) {}

void modle_config_edit::init_(const entt::handle& in) {
  p_i->regex_ = in.get_or_emplace<project_config::model_config>().find_icon_regex;
}
void modle_config_edit::render(const entt::handle& in) {
  if (ImGui::InputText(*p_i->regex_.gui_name, &p_i->regex_.data, ImGuiInputTextFlags_EnterReturnsTrue)) {
    try {
      std::regex l_regex{p_i->regex_.data};
      set_modify(true);
      p_i->err_str.clear();
    } catch (const std::regex_error& error) {
      p_i->err_str = fmt::format("错误的正则表达式 {}", p_i->regex_.data);
      DOODLE_LOG_ERROR(p_i->err_str)
    }
  };
  if (!p_i->err_str.empty()) {
    dear::Text(p_i->err_str);
  }
}
void modle_config_edit::save_(const entt::handle& in) const {
  in.get<project_config::model_config>().find_icon_regex = p_i->regex_;
}
modle_config_edit::~modle_config_edit() = default;
}  // namespace doodle::gui
