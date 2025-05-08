//
// Created by TD on 2022/8/4.
//
#pragma once

#include <doodle_core/doodle_core.h>

#include <bitset>
#include <chrono>
#include <utility>

namespace doodle::business {

/**
 * @brief 这个时间规则是一个本地时间(并非 utc 时间)
 */

class DOODLE_CORE_API rules {
 public:
  using duration_type        = chrono::seconds;

  using time_duration_vector = std::vector<std::pair<duration_type, duration_type>>;
  /// \brief 周六 ->周日(index 6->0)
  using work_day_type        = std::bitset<7>;

 private:
  friend void to_json(nlohmann::json& j, const rules& p);
  friend void from_json(const nlohmann::json& j, rules& p);

 public:
  /// @brief 工作时间 9:00 - 12:00, 13:00 - 18:00
  time_duration_vector work_pair_p{};
  /// @brief  非工作日休息时间 12:00 - 13:00, 18:30 - 19:00
  time_duration_vector work_pair_0_{};
  /// @brief  工作日休息时间 12:00 - 13:00, 18:00 - 18:30
  time_duration_vector work_pair_1_{};

  rules();
  virtual ~rules();

  /**
   * @brief 工作时间的默认时间段
   * @return 默认段规则
   */
  [[nodiscard("")]] static const rules& get_default();
};

}  // namespace doodle::business
