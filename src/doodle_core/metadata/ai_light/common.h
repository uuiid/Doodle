#pragma once

#include <doodle_core/doodle_core_fwd.h>

namespace doodle::ai_light {

struct DOODLE_CORE_API interval_t {
  double low_{0.0};
  double high_{0.0};

  bool contains(double in_value) const { return in_value >= low_ && in_value <= high_; }
  double center() const { return (low_ + high_) * 0.5; }
  double width() const { return high_ - low_; }

  friend void to_json(nlohmann::json& j, const interval_t& p) { j = nlohmann::json::array({p.low_, p.high_}); }
  friend void from_json(const nlohmann::json& j, interval_t& p) {
    if (!j.is_array() || j.size() != 2)
      throw nlohmann::json::type_error::create(302, "interval_t 需要长度为 2 的数组", &j);
    j.at(0).get_to(p.low_);
    j.at(1).get_to(p.high_);
  }
};

}  // namespace doodle::ai_light
