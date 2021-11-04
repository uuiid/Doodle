//
// Created by TD on 2021/11/04.
//

#include "opencv_player.h"

#include <doodle_lib/core/doodle_lib.h>
#include <doodle_lib/file_warp/opencv_read_player.h>
namespace doodle {
opencv_player::opencv_player() {
  p_class_name = "视频播放";
}

void opencv_player::frame_render() {
  auto k_view = g_reg()->view<opencv_read_player>();
  for (auto& k_i : k_view) {
    auto& k_open = k_view.get<opencv_read_player>(k_i);
    if (!k_open.is_open())
      continue;

    auto [k_v, k_s] = k_open.read(1);
    imgui::Image(k_v, ImVec2{
                          boost::numeric_cast<std::float_t>(k_s.first),
                          boost::numeric_cast<std::float_t>(k_s.second)});
  }
}

}  // namespace doodle