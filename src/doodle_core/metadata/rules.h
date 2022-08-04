//
// Created by TD on 2022/8/4.
//
#pragma once

#include <doodle_core/doodle_core.h>

#include <bitset>
#include <utility>

namespace doodle::business {
class rules;
void to_json(nlohmann::json& j, const rules& p);
void from_json(const nlohmann::json& j, rules& p);

class rules {
 private:
  friend void to_json(nlohmann::json& j, const rules& p);
  friend void from_json(const nlohmann::json& j, rules& p);
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  /// \brief 周六 ->周日(index 6->0)
  constexpr static std::bitset<7> work_Monday_to_Friday{0b0111110};
  constexpr static std::pair<chrono::seconds,
                             chrono::seconds>
      work_9_12{9h, 12h};
  constexpr static std::pair<chrono::seconds,
                             chrono::seconds>
      work_13_18{13h, 18h};
  rules();
  virtual ~rules();
};

}  // namespace doodle::business
