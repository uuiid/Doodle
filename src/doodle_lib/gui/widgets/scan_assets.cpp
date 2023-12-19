//
// Created by TD on 2023/12/19.
//

#include "scan_assets.h"

#include <doodle_app/lib_warp/imgui_warp.h>
namespace doodle::gui {

namespace details {}

void scan_assets_t::start_scan() {}

bool scan_assets_t::render() {
  if (ImGui::Button(*start_scan_id)) {
    start_scan();
  }
  return is_open;
}
}  // namespace doodle::gui