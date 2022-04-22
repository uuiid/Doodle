//
// Created by TD on 2022/4/1.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <bitset>

namespace doodle {

namespace business {

class DOODLELIB_API adjust_work {
 public:
  chrono::local_time_pos start_;
  chrono::local_time_pos end_;
};

class DOODLELIB_API adjust_rest {
 public:
  chrono::local_time_pos start_;
  chrono::local_time_pos end_;
};

class DOODLELIB_API rules {
 public:
  /// \brief 工作日 从周一到周日
  std::bitset<7> work_weekdays;
  std::set<
      std::pair<
          chrono::seconds,
          chrono::seconds>>
      work_pair;
  std::vector<adjust_work> extra_work;
  std::vector<adjust_rest> extra_rest;
  chrono::local_time_pos get_work_time(const chrono::local_time_pos& in_s);

  /**
   * @brief 获取当天的工作时间段
   * @param in_day 传入的天数
   * @return
   */
  std::vector<std::pair<
      chrono::seconds,
      chrono::seconds>>
  operator()(const chrono::year_month_day& in_day);
};

}  // namespace business

chrono::hours_double work_duration(
    const chrono::local_time_pos& in_s,
    const chrono::local_time_pos& in_e,
    const business::rules& in_rules);

chrono::local_time_pos next_time(
    const chrono::local_time_pos& in_s,
    const chrono::milliseconds& in_du_time,
    const business::rules& in_rules);
}  // namespace doodle
