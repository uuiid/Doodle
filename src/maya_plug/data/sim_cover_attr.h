//
// Created by TD on 2022/1/7.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {
class sim_cover_attr;
void to_json(nlohmann::json& j, const sim_cover_attr& p);
void from_json(const nlohmann::json& j, sim_cover_attr& p);
class sim_cover_attr {
 private:
 public:
  sim_cover_attr() = default;

  bool simple_subsampling{true};

  std::int32_t frame_samples{6};
  std::float_t time_scale{1.0f};
  std::float_t length_scale{1.0f};
  std::int32_t max_cg_iteration{1000};
  std::int32_t cg_accuracy{9};
  std::array<std::float_t, 3> gravity{0.0f, -980.0f, 0.0f};

  static void cover_qcloth_attr(const entt::handle& in_entity);

 private:
  friend void to_json(nlohmann::json& j, const sim_cover_attr& p);
  friend void from_json(const nlohmann::json& j, sim_cover_attr& p);
};
}  // namespace doodle::maya_plug
