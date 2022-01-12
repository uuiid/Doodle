//
// Created by TD on 2021/10/25.
//

#include "command_video.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/metadata_cpp.h>
namespace doodle {

comm_video::comm_video()
    : command_base(),
      p_root() {
  p_name     = "拍屏";
  p_show_str = make_imgui_name(this, "观看拍屏");
}
bool comm_video::render() {
  if (p_root) {
    if (imgui::Button(p_show_str["观看拍屏"].c_str())) {
      auto& k_f   = p_root.get<assets_path_vector>();

    }
  }
  return false;
}
bool comm_video::set_data(const entt::handle& in_any) {
  if (in_any.all_of<assets_path_vector>()) {
    p_root = in_any;
  } else {
    p_root = entt::handle{};
  }

  return false;
}
}  // namespace doodle
