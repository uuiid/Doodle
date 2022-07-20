//
// Created by TD on 2022/7/20.
//

#include "qcloth_sim_attr.h"

namespace doodle {
void to_json(nlohmann::json& j, const qcloth_sim_attr& p) {
  j["simple_subsampling"] = p.simple_subsampling;
  j["frame_samples"]      = p.frame_samples;
  j["time_scale"]         = p.time_scale;
  j["length_scale"]       = p.length_scale;
  j["max_cg_iteration"]   = p.max_cg_iteration;
  j["cg_accuracy"]        = p.cg_accuracy;
}
void from_json(const nlohmann::json& j, qcloth_sim_attr& p) {
  j.at("simple_subsampling").get_to(p.simple_subsampling);
  j.at("frame_samples").get_to(p.frame_samples);
  j.at("time_scale").get_to(p.time_scale);
  j.at("length_scale").get_to(p.length_scale);
  j.at("max_cg_iteration").get_to(p.max_cg_iteration);
  j.at("cg_accuracy").get_to(p.cg_accuracy);
}
}  // namespace doodle
