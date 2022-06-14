//
// Created by TD on 2022/4/1.
//

#include "work_clock.h"
#include <date/tz.h>
#include <boost/contract.hpp>

namespace doodle {

namespace business {

std::vector<time_attr> rules::operator()(
    const chrono::year_month_day& in_day) const {
  //  ::boost::contract::check _l_c =
  //      ::boost::

  chick_true<doodle_error>(in_day.ok(), DOODLE_LOC, "无效的日期 {}", in_day);
  chrono::weekday l_weekday{in_day};
  std::vector<time_attr> time_list{};

  //  std::vector<std::pair<chrono::seconds, chrono::seconds>> result{};
  /// \brief 加入工作日规定时间
  chrono::local_days l_local_days{in_day};
  if (work_weekdays[l_weekday.c_encoding()]) {
    ranges::for_each(work_pair, [&](const std::pair<chrono::seconds,
                                                    chrono::seconds>& in_pair) {
      time_list.emplace_back(l_local_days + in_pair.first, work_attr::normal_work_begin);
      time_list.emplace_back(l_local_days + in_pair.second, work_attr::normal_work_end);
    });
  }

  /// 开始加入调休和加班
  //  ranges::for_each(extra_work,
  //                   [&](const decltype(extra_work)::value_type& in_work) {
  //                     time_list.emplace_back(in_work.start_, work_attr::adjust_work_begin);
  //                     time_list.emplace_back(in_work.end_, work_attr::adjust_work_end);
  //                   });
  //  ranges::for_each(extra_rest,
  //                   [&](const decltype(extra_rest)::value_type& in_rest) {
  //                     time_list.emplace_back(in_rest.start_, work_attr::adjust_rest_begin);
  //                     time_list.emplace_back(in_rest.end_, work_attr::adjust_rest_end);
  //                   });

  time_list |= ranges::actions::sort;
  return time_list;
  //  const chrono::local_days l_days{in_day};
  //
  //  /// \brief 对时间进行整理
  //  for (auto l_item : time_list) {
  //    auto l_time = chrono::floor<chrono::seconds>(l_item.time_point - l_days);
  //    std::optional<chrono::seconds> start{};
  //    std::optional<chrono::seconds> end{};
  //
  //    if (l_item.state_ == time_attr::work_begin) {
  //      if (!start)
  //        *start = l_time;
  //    } else if (l_item.state_ == time_attr::work_end) {
  //      if (!end)
  //        *end = l_time;
  //
  //    } else if (l_item.state_ == time_attr::rest_begin) {
  //      if (!end)
  //        *end = l_time;
  //    } else if (l_item.state_ == time_attr::rest_end) {
  //      if (!start)
  //        *start = l_time;
  //    }
  //    if (start && end) {
  //      result.emplace_back(*start, *end);
  //      start.reset();
  //      end.reset();
  //    }
  //  }
  //
  //  return result;
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
void time_attr::add_event(boost::msm::back::state_machine<detail::work_machine_front>& in_mfm) {
  if (state_ == work_attr::normal_work_begin) {
    in_mfm.process_event(doodle::business::detail::normal_work_begin{time_point});
  } else if (state_ == work_attr::normal_work_end) {
    in_mfm.process_event(doodle::business::detail::normal_work_end{time_point});
  } else if (state_ == work_attr::adjust_work_begin) {
    in_mfm.process_event(doodle::business::detail::adjust_work_begin{time_point});
  } else if (state_ == work_attr::adjust_work_end) {
    in_mfm.process_event(doodle::business::detail::adjust_work_end{time_point});
  } else if (state_ == work_attr::adjust_rest_begin) {
    in_mfm.process_event(doodle::business::detail::adjust_rest_begin{time_point});
  } else if (state_ == work_attr::adjust_rest_end) {
    in_mfm.process_event(doodle::business::detail::adjust_rest_end{time_point});
  }
}
void detail::work_machine_front::add_time(const chrono::local_time_pos& in_time) {
  work_time_       = (in_time - time_);
  auto l_time_long = in_time - time_;
  if (work_limit_) {
    if ((work_time_ + l_time_long) > *work_limit_) {
      time_ += (*work_limit_ - work_time_);
      work_time_ = *work_limit_;
    } else {
      time_ = in_time;
      work_time_ += l_time_long;
    }
  } else {
    time_ = in_time;
    work_time_ += l_time_long;
  }
}
bool detail::work_machine_front::ok() const {
  if (work_limit_) {
    return *work_limit_ == work_time_;
  } else
    return true;
}
void detail::work_machine_front::set_work_limit(const chrono::local_time_pos& in_pos, const chrono::seconds& in_work_du) {
  work_limit_ = in_work_du;
  time_       = in_pos;
}
}  // namespace business
namespace detail {

chrono::local_time_pos next_time(const chrono::local_time_pos& in_s,
                                 const std::chrono::milliseconds& in_du_time,
                                 const business::rules& in_rules) {
  auto l_day_1   = chrono::year_month_day{chrono::floor<chrono::days>(in_s)};
  auto l_day_end = chrono::local_days{chrono::year_month_day_last{
                       l_day_1.year(),
                       chrono::month_day_last{l_day_1.month()}}} +
                   720h;

  business::detail::work_clock_mfm l_mfm{};
  l_mfm.start();
  l_mfm.set_work_limit(in_s, chrono::floor<chrono::seconds>(in_du_time));

  for (auto l_day = chrono::floor<chrono::days>(in_s);
       l_day < l_day_end;
       l_day += chrono::days{1}) {
    auto l_r = in_rules(chrono::year_month_day{l_day});
    for (auto&& i : l_r) {
      i.add_event(l_mfm);
      if (l_mfm.ok()) {
        return l_mfm.time_;
      }
    }
  }
  return l_mfm.time_;
}
chrono::hours_double work_duration(const chrono::local_time_pos& in_s,
                                   const chrono::local_time_pos& in_e,
                                   const business::rules& in_rules) {
  const auto l_day_end = chrono::floor<chrono::days>(in_e);

  business::detail::work_clock_mfm l_mfm{};
  l_mfm.start();
  for (auto l_day = chrono::floor<chrono::days>(in_s);
       l_day <= l_day_end;
       l_day += chrono::days{1}) {
    auto l_r = in_rules(chrono::year_month_day{l_day});
    for (auto&& i : l_r) {
      if (i.time_point <= in_e) {
        i.add_event(l_mfm);
      } else {
        i.time_point = in_e;
        i.add_event(l_mfm);
        return l_mfm.work_time_;
      }
    }
  }

  return l_mfm.work_time_;
}
}  // namespace detail

}  // namespace doodle
