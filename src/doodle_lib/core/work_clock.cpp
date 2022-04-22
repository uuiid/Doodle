//
// Created by TD on 2022/4/1.
//

#include "work_clock.h"
#include <date/tz.h>
#include <boost/contract.hpp>

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
  //  ::boost::contract::check _l_c =
  //      ::boost::

  chick_true<doodle_error>(in_day.ok(), DOODLE_LOC, "无效的日期 {}", in_day);
  chrono::weekday l_weekday{in_day};
  std::vector<time_attr> time_list{};

  std::vector<std::pair<chrono::seconds, chrono::seconds>> result{};
  /// \brief 加入工作日规定时间
  if (work_weekdays[l_weekday.c_encoding() - 1]) {
    ranges::for_each(work_pair, [&](const std::pair<chrono::seconds,
                                                    chrono::seconds>& in_pair) {
      time_list.emplace_back(in_pair.first, time_attr::work_begin);
      time_list.emplace_back(in_pair.second, time_attr::work_end);
    });
  }
  /// 开始加入调休和加班
  ranges::for_each(extra_work,
                   [&](const decltype(extra_work)::value_type& in_work) {
                     time_list.emplace_back(in_work.start_, time_attr::work_begin);
                     time_list.emplace_back(in_work.end_, time_attr::work_end);
                   });
  ranges::for_each(extra_rest,
                   [&](const decltype(extra_rest)::value_type& in_rest) {
                     time_list.emplace_back(in_rest.start_, time_attr::rest_begin);
                     time_list.emplace_back(in_rest.end_, time_attr::rest_end);
                   });

  time_list |= ranges::actions::sort;
  const chrono::local_days l_days{in_day};

  /// \brief 对时间进行整理
  for (auto l_item : time_list) {
    auto l_time = chrono::floor<chrono::seconds>(l_item.time_point - l_days);
    std::optional<chrono::seconds> start{};
    std::optional<chrono::seconds> end{};

    if (l_item.state_ == time_attr::work_begin) {
      if (!start)
        *start = l_time;
    } else if (l_item.state_ == time_attr::work_end) {
      if (!end)
        *end = l_time;

    } else if (l_item.state_ == time_attr::rest_begin) {
      if (!end)
        *end = l_time;
    } else if (l_item.state_ == time_attr::rest_end) {
      if (!start)
        *start = l_time;
    }
    if (start && end) {
      result.emplace_back(*start, *end);
      start.reset();
      end.reset();
    }
  }

  return result;
}
bool time_attr::operator<(const time_attr& in_rhs) const {
  return time_point < in_rhs.time_point;
}
bool time_attr::operator>(const time_attr& in_rhs) const {
  return in_rhs < *this;
}
bool time_attr::operator<=(const time_attr& in_rhs) const {
  return !(in_rhs < *this);
}
bool time_attr::operator>=(const time_attr& in_rhs) const {
  return !(*this < in_rhs);
}
bool time_attr::operator==(const time_attr& in_rhs) const {
  return time_point == in_rhs.time_point &&
         state_ == in_rhs.state_;
}
bool time_attr::operator!=(const time_attr& in_rhs) const {
  return !(in_rhs == *this);
}
}  // namespace business
chrono::local_time_pos next_time(const chrono::local_time_pos& in_s,
                                 const std::chrono::milliseconds& in_du_time,
                                 const business::rules& in_rules) {
  return chrono::local_time_pos();
}

}  // namespace doodle
