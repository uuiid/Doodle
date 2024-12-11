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
  /// \brief 周六 ->周日(index 6->0)
  constexpr static work_day_type work_Monday_to_Friday{0b0111110};
  /// @brief 周六到周日每日必然会排除的时间
  work_day_type work_weekdays_p{};
  time_duration_vector work_pair_p{};
  std::array<time_duration_vector, 7> absolute_deduction{};

  rules();
  virtual ~rules();

  /**
   * @brief 获取每周 1-5工作, 每天9-12 13-18 工作时间的默认时间段
   * @return 默认段规则
   */
  [[nodiscard("")]] static const rules& get_default();
  bool is_work_day(const chrono::weekday& in_week_day) const;
};

}  // namespace doodle::business

