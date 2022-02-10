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

}  // namespace doodle
