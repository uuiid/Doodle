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


namespace doodle {
edit_widgets::edit_widgets()
    : p_meta_parent(),
    p_reg(g_reg()) {
  p_class_name = "编辑";
}

void edit_widgets::frame_render() {
  auto k_ctx = p_reg->try_ctx<widget_>();
  if (k_ctx)
    k_ctx->render();
}

}  // namespace doodle
