//
// Created by TD on 2022/7/20.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
namespace doodle {
class qcloth_sim_attr;
void to_json(nlohmann::json& j, const qcloth_sim_attr& p);
void from_json(const nlohmann::json& j, qcloth_sim_attr& p);

class DOODLE_CORE_EXPORT qcloth_sim_attr {
 public:
  bool simple_subsampling{true};

  std::int32_t frame_samples{6};
  std::float_t time_scale{1.0f};
  std::float_t length_scale{1.0f};
  std::int32_t max_cg_iteration{1000};
  std::int32_t cg_accuracy{9};

  qcloth_sim_attr() = default;

 private:
  friend void to_json(nlohmann::json& j, const qcloth_sim_attr& p);
  friend void from_json(const nlohmann::json& j, qcloth_sim_attr& p);
};

}  // namespace doodle
