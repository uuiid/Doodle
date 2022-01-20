//
// Created by TD on 2022/1/20.
//

#include "path.h"
#include <lib_warp/imgui_warp.h>

namespace doodle::gui {
bool adl_render<FSys::path>::render(FSys::path& in_path) {
  if (dear::InputText("路径", &data)) {
    in_path = data;
    return true;
  }
  return false;
}
}  // namespace doodle::gui
