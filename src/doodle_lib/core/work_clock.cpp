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

  /// \brief 添加加班时间
  for (const auto& i : extra_work) {
    const auto l_to = chrono::floor<chrono::days>(i.start_);
    chrono::year_month_day l_day{l_to};
    /// \brief 判断加班时间是否在当天
    if (l_day == in_day) {
      auto l_ss      = chrono::floor<chrono::milliseconds>(i.start_ - l_to);
      /// \brief 寻找加班时间和工作时间重叠
      auto l_work_it = ranges::find_if(
          result,
          [&](const std::pair<chrono::seconds,
                              chrono::seconds>& in_pair) -> bool {
            return in_pair.second > l_ss;
          });
      /// \brief 重叠延长工作时间
      if (l_work_it != result.end()) {
        if (l_work_it->first < i.start_) {
          l_work_it->first = i.start_;
        }
        l_work_it->second = i.end_ - l_to;
      } else {
        /// \brief 非重叠加入加班时间
        result.emplace_back(std::make_pair(i.start_ - l_to, i.end_ - l_to));
      }
    }
  }
  /// \brief 添加调休时间
  for (const auto& i : extra_rest) {
    const auto l_to = chrono::floor<chrono::days>(i.start_);
    chrono::year_month_day l_day{l_to};
    /// \brief 判断调休时间是否在当天
    if (l_day == in_day) {
      /// \brief 开始寻找调休重叠时间
      auto l_ss      = chrono::floor<chrono::milliseconds>(i.start_ - l_to);
      auto l_rest_it = ranges::find_if(
          result,
          [&](const std::pair<chrono::seconds,
                              chrono::seconds>& in_pair) -> bool {
            return in_pair.second > l_ss;
          });

      /// \brief 如果有重叠 work(13-18)
      if (l_rest_it != result.end()) {
        /// \brief 调休时间大于工作时间 12-19
        if (l_rest_it->first > i.start_ && l_rest_it->second < i.end_) {
        } else if (l_rest_it->first < i.start_ && l_rest_it->second > i.end_) {
          /// \brief 包含重叠 14-15
        } else if (l_rest_it->first < i.start_ && l_rest_it->second < i.end_) {
          /// \brief 部分重叠 14-20
        } else if (l_rest_it) {
        }
        /// \brief 部分重叠2 12:30-14
      }
    }
  }

  return result;
}
}  // namespace business
chrono::local_time_pos next_time(const chrono::local_time_pos& in_s,
                                 const std::chrono::milliseconds& in_du_time,
                                 const business::rules& in_rules) {
  return chrono::local_time_pos();
}

}  // namespace doodle
