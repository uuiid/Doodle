//
// Created by TD on 2022/8/4.
//
#pragma once

#include <doodle_core/doodle_core.h>

#include <bitset>
#include <utility>

namespace doodle {
class time_point_wrap;
}

namespace doodle::business {
class rules;
void to_json(nlohmann::json& j, const rules& p);
void from_json(const nlohmann::json& j, rules& p);

namespace rules_ns {
class time_point_info;
};

class rules : boost::noncopyable {
 private:
  friend void to_json(nlohmann::json& j, const rules& p);
  friend void from_json(const nlohmann::json& j, rules& p);
  class impl;
  std::unique_ptr<impl> p_i;

 public:
  rules();
  virtual ~rules();

  rules(rules&& in_rules) noexcept;
  rules& operator=(rules&& in_rules) noexcept;
  /**
   * @brief 获取每周 1-5工作, 每天9-12 13-18 工作时间的默认时间段
   * @return 默认段规则
   */
  [[nodiscard("")]] static rules get_default();

  /// \brief 周六 ->周日(index 6->0)
  using work_day_type = std::bitset<7>;

  void work_weekdays(const work_day_type& in_work_weekdays);
  [[nodiscard("")]] const work_day_type& work_weekdays() const;

  void add_work_time(const chrono::seconds& in_begin, const chrono::seconds& in_end);
  [[nodiscard("")]] const std::vector<std::pair<chrono::seconds, chrono::seconds>>& work_time() const;

  void add_extra_work(const time_point_wrap& in_begin, const time_point_wrap& in_end, const std::string& in_info);
  [[nodiscard("")]] const std::vector<rules_ns::time_point_info>& extra_work() const;

  void add_extra_rest(const time_point_wrap& in_begin, const time_point_wrap& in_end, const std::string& in_info);
  [[nodiscard("")]] const std::vector<rules_ns::time_point_info>& extra_rest() const;
};

}  // namespace doodle::business
