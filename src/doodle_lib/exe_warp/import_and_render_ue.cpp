//
// Created by TD on 2024/1/8.
//

#include "import_and_render_ue.h"
#include <doodle_lib/exe_warp/ue_exe.h>
namespace doodle {

import_and_render_ue::import_and_render_ue() {
  if (!g_ctx().contains<ue_exe>()) g_ctx().emplace<ue_exe>();
}
}  // namespace doodle