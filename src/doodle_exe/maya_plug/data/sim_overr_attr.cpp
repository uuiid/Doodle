//
// Created by TD on 2022/1/7.
//

#include "sim_overr_attr.h"
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
namespace doodle {
namespace maya_plug {

sim_overr_attr::sim_overr_attr()
    : simple_subsampling(true),
      frame_samples(10),
      time_scale(1),
      length_scale(1),
      sharp_feature(true) {
}
}  // namespace maya_plug

namespace gui {
bool adl_render<maya_plug::sim_overr_attr>::render(
    const entt::handle& in_handle) {
  auto& in_attr = in_handle.get<maya_plug::sim_overr_attr>();
  dear::Checkbox("精密解算", &in_attr.simple_subsampling);
  dear::SliderFloat("帧样本", &in_attr.frame_samples, 0.1f, 50.f);
  dear::SliderFloat("时间尺度", &in_attr.time_scale, 0.1f, 10.f);
  dear::SliderFloat("长度尺度", &in_attr.length_scale, 0.1f, 10.f);
  dear::Checkbox("尖锐碰撞", &in_attr.sharp_feature);
  return true;
}
}  // namespace gui
}  // namespace doodle
