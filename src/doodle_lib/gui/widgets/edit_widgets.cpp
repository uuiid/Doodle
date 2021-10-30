//
// Created by TD on 2021/9/23.
//

#include "edit_widgets.h"

#include <doodle_lib/gui/action/command.h>
#include <doodle_lib/gui/action/command_files.h>
#include <doodle_lib/gui/action/command_meta.h>
#include <doodle_lib/gui/action/command_ue4.h>
#include <doodle_lib/gui/action/command_video.h>
#include <doodle_lib/gui/factory/attribute_factory_interface.h>
#include <doodle_lib/metadata/metadata.h>

#define DOODLE_ENTT_RENDER(class_name)    \
  {                                       \
    auto k_v = k_reg->view<class_name>(); \
    for (auto k_i : k_v) {                \
      k_v.get<class_name>(k_i).render();  \
    }                                     \
  }

namespace doodle {
edit_widgets::edit_widgets()
    : p_meta_parent() {
  p_class_name = "编辑";
}

void edit_widgets::frame_render() {
  auto k_reg = g_reg();
  // {
  //   auto k_v = k_reg->view<comm_project_add>();
  //   for (auto& k_i : k_v) {
  //     k_v.get<comm_project_add>(k_i).render();
  //   }
  // }
  DOODLE_ENTT_RENDER(comm_project_add);
  DOODLE_ENTT_RENDER(comm_ass_season);
  DOODLE_ENTT_RENDER(comm_ass_eps);
  DOODLE_ENTT_RENDER(comm_ass_shot);
  DOODLE_ENTT_RENDER(comm_assets);
  DOODLE_ENTT_RENDER(comm_ass_ue4_create_shot);
}

void edit_widgets::set_factort(const attribute_factory_ptr& in_factory) {
}
}  // namespace doodle
