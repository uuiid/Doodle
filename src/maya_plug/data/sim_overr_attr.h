//
// Created by TD on 2022/1/7.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
namespace maya_plug {
class sim_overr_attr;
void to_json(nlohmann::json& j, const sim_overr_attr& p);
void from_json(const nlohmann::json& j, sim_overr_attr& p);
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
  friend void to_json(nlohmann::json& j, const sim_overr_attr& p);
  friend void from_json(const nlohmann::json& j, sim_overr_attr& p);
};
}  // namespace maya_plug

}  // namespace doodle
