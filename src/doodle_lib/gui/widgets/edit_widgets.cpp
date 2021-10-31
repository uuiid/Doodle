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
  render_<comm_project_add,
          comm_ass_season,
          comm_ass_eps,
          comm_ass_shot,
          comm_assets,
          comm_ass_ue4_create_shot>();
}

void edit_widgets::set_factort(const attribute_factory_ptr& in_factory) {
}
}  // namespace doodle
