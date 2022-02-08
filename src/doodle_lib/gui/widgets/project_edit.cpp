//
// Created by TD on 2022/2/7.
//

#include "project_edit.h"
#include <metadata/project.h>
#include <doodle_lib/lib_warp/imgui_warp.h>

namespace doodle {

class project_edit::impl {
 public:
  impl() = default;
  entt::handle p_h;

  project p_prj;
  std::string path;
  project::cloth_config cloth_config;
  std::string cloth_config_path;
};

project_edit::project_edit()
    : p_i(std::make_unique<impl>()) {
}
project_edit::~project_edit() = default;

void project_edit::init() {
  p_i->p_h               = project::get_current();

  p_i->p_prj             = p_i->p_h.get<project>();
  p_i->path              = p_i->p_prj.p_path.generic_string();
  p_i->cloth_config      = p_i->p_h.get_or_emplace<project::cloth_config>();
  p_i->cloth_config_path = p_i->cloth_config.vfx_cloth_sim_path.generic_string();
}
void project_edit::succeeded() {
}
void project_edit::failed() {
}
void project_edit::aborted() {
}
void project_edit::update(const chrono::duration<chrono::system_clock::rep,
                                                 chrono::system_clock::period>&,
                          void* data) {
  ImGui::Text("基本配置");
  dear::Text(fmt::format("路径: {}", p_i->path));

  if (ImGui::InputText("名称", &p_i->p_prj.p_name)) {
    p_i->p_h.patch<project>([&](project& in) {
      in.set_name(p_i->p_prj.p_name);
    });
    g_reg()->set<project>(p_i->p_h.get<project>());
  }

  dear::TreeNode{"解算配置"} &&
      [&]() {
        if (imgui::InputText("解算路径", &(p_i->cloth_config_path))) {
          p_i->p_h.patch<project::cloth_config>([&](project::cloth_config& in) {
            in.vfx_cloth_sim_path = p_i->cloth_config_path;
          });
        }

        if (imgui::InputText("导出节点", &(p_i->cloth_config.export_group))) {
          p_i->p_h.patch<project::cloth_config>([&](project::cloth_config& in) {
            in.export_group = p_i->cloth_config.export_group;
          });
        }
      };
}
}  // namespace doodle
