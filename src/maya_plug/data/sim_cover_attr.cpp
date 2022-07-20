
//
// Created by TD on 2022/1/7.
//

#include "sim_cover_attr.h"
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/lib_warp/imgui_warp.h>
namespace doodle::maya_plug {
void to_json(nlohmann::json& j, const sim_cover_attr& p) {
  j["simple_subsampling"] = p.simple_subsampling;
  j["frame_samples"]      = p.frame_samples;
  j["time_scale"]         = p.time_scale;
  j["length_scale"]       = p.length_scale;
  j["max_cg_iteration"]   = p.max_cg_iteration;
  j["cg_accuracy"]        = p.cg_accuracy;
}
void from_json(const nlohmann::json& j, sim_cover_attr& p) {
  if (j.contains("simple_subsampling"))
    j.at("simple_subsampling").get_to(p.simple_subsampling);
  if (j.contains("frame_samples"))
    j.at("frame_samples").get_to(p.frame_samples);
  if (j.contains("time_scale"))
    j.at("time_scale").get_to(p.time_scale);
  if (j.contains("length_scale"))
    j.at("length_scale").get_to(p.length_scale);
  if (j.contains("max_cg_iteration"))
    j.at("max_cg_iteration").get_to(p.max_cg_iteration);
  if (j.contains("cg_accuracy"))
    j.at("cg_accuracy").get_to(p.cg_accuracy);
}
}  // namespace doodle
