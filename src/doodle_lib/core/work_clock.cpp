//
// Created by TD on 2022/4/1.
//

#include "work_clock.h"
#include <date/tz.h>

namespace doodle {

namespace business {

chrono::local_time_pos rules::get_work_time(const chrono::local_time_pos& in_s) {
  auto l_time = chrono::make_zoned(chrono::current_zone(), in_s);
  /// \brief 首先查看在那天开始工作

  return doodle::chrono::local_time_pos();
}
std::vector<std::pair<
    chrono::seconds,
    chrono::seconds>>
rules::operator()(const chrono::year_month_day& in_day) {
  /// \brief 本年本月的最后一天

  chrono::weekday l_weekday{in_day};
  std::vector<std::pair<chrono::seconds, chrono::seconds>> result{};
  if (work_weekdays[l_weekday.c_encoding() - 1]) {
    result.insert(result.begin(), work_pair.begin(), work_pair.end());
  }
  for (const auto& i : extra_work) {
    //    chrono::year_month_day l_day{i.start_};

    for (auto& j : result) {
    }
  }

  return std::vector<std::pair<chrono::seconds, chrono::seconds>>();
}
}  // namespace business
chrono::local_time_pos next_time(const chrono::local_time_pos& in_s,
                                 const std::chrono::milliseconds& in_du_time,
                                 const business::rules& in_rules) {
  return chrono::local_time_pos();
}

}  // namespace doodle
