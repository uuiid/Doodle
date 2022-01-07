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
  sim_overr_attr();
  //  /**
  //   * @brief 调整参数
  //   */
  //  bool use_overr_attr;
  /**
   * @brief 精密解算
   */
  bool simple_subsampling;
  /**
   * @brief 帧样本 1.f <-> 50.f
   */
  std::float_t frame_samples;
  /**
   * @brief 时间尺度 0.1f <-> 10.f
   */
  std::float_t time_scale;
  /**
   * @brief 长度尺度 0.1f <-> 10.f
   */
  std::float_t length_scale;
  /**
   * @brief 尖锐碰撞 true
   */
  bool sharp_feature{};

 private:
  NLOHMANN_DEFINE_TYPE_INTRUSIVE(sim_overr_attr,
                                 simple_subsampling,
                                 frame_samples,
                                 time_scale,
                                 length_scale,
                                 sharp_feature);
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
namespace gui {
template <>
struct adl_render<maya_plug::sim_overr_attr> {
  static bool render(const entt::handle& in_handle);
};

}  // namespace gui
}  // namespace doodle
