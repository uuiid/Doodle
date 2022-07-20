//
// Created by TD on 2022/1/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
namespace maya_plug {

class sim_overr_attr {
 private:
 public:
  sim_overr_attr() = default;

  bool simple_subsampling{true};

  std::int32_t frame_samples{6};
  std::float_t time_scale{1.0f};
  std::float_t length_scale{1.0f};
  std::int32_t max_cg_iteration{1000};
  std::int32_t cg_accuracy{9};

 private:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(sim_overr_attr,
                                 frame_samples,
                                 time_scale,
                                 length_scale,
                                 max_cg_iteration,
                                 cg_accuracy);
  //  friend void to_json(nlohmann::json& j, const sim_overr_attr& p) {
  //    j["simple_subsampling"] = p.simple_subsampling;
  //    j["frame_samples"]      = p.frame_samples;
  //    j["time_scale"]         = p.time_scale;
  //    j["length_scale"]       = p.length_scale;
  //    j["sharp_feature"]      = p.sharp_feature;
  //  }
  //
  //  friend void from_json(const nlohmann::json& j, sim_overr_attr& p) {
  //    j.at("simple_subsampling").get_to(p.simple_subsampling);
  //    j.at("frame_samples").get_to(p.frame_samples);
  //    j.at("time_scale").get_to(p.time_scale);
  //    j.at("length_scale").get_to(p.length_scale);
  //    j.at("sharp_feature").get_to(p.sharp_feature);
  //  }
};
}  // namespace maya_plug

}  // namespace doodle
