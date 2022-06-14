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
  ranges::for_each(extra_work,
                   [&](const decltype(extra_work)::value_type& in_work) {
                     time_list.emplace_back(in_work.start_, work_attr::adjust_work_begin);
                     time_list.emplace_back(in_work.end_, work_attr::adjust_work_end);
                   });
  ranges::for_each(extra_rest,
                   [&](const decltype(extra_rest)::value_type& in_rest) {
                     time_list.emplace_back(in_rest.start_, work_attr::adjust_rest_begin);
                     time_list.emplace_back(in_rest.end_, work_attr::adjust_rest_end);
                   });

  time_list |= ranges::actions::sort;
  return time_list;
}
void rules::clamp_time(chrono::local_time_pos& in_time_pos) const {
  auto l_day = chrono::year_month_day{chrono::floor<chrono::days>(in_time_pos)};
  chick_true<doodle_error>(l_day.ok(), DOODLE_LOC, "无效的日期 {}", l_day);
  chrono::weekday l_weekday{l_day};

  /// \brief 加入工作日规定时间
  chrono::local_days l_local_days{l_day};
  if (work_weekdays[l_weekday.c_encoding()]) {
    auto l_r = ranges::any_of(work_pair,
                              [&](const std::pair<chrono::seconds, chrono::seconds>& in_pair) -> bool {
                                return in_time_pos >= (l_local_days + in_pair.first) &&
                                       in_time_pos <= (l_local_days + in_pair.second);
                              });

    if (!l_r && !work_pair.empty()) {
      in_time_pos = (l_local_days + work_pair.back().second);
    }
  } else {
    for (auto i = 0; i < work_weekdays.size(); ++i) {
      if (work_weekdays[l_weekday.c_encoding() + i]) {
        in_time_pos = ((l_local_days + doodle::chrono::days{i}) + work_pair.front().first);
      }
    }
  }
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
void time_attr::add_event(doodle::business::detail::work_clock_mfm_base& in_mfm) {
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
  auto l_time_long = in_time - time_;
  time_            = in_time;
  work_time_ += l_time_long;
}

chrono::hours_double detail::work_clock_mfm::work_duration(
    const chrono::local_time_pos& in_e,
    const rules& in_rules) {
  const auto l_day_end = chrono::floor<chrono::days>(in_e);

  for (auto l_day = chrono::floor<chrono::days>(time_);
       l_day <= l_day_end;
       l_day += chrono::days{1}) {
    auto l_r = in_rules(chrono::year_month_day{l_day});
    for (auto&& i : l_r) {
      if (i.time_point <= in_e) {
        i.add_event(*this);
      } else {
        i.time_point = in_e;
        i.add_event(*this);
        return work_time_;
      }
    }
  }

  return work_time_;
}

void detail::work_next_clock_mfm::add_time(const chrono::local_time_pos& in_time) {
  auto l_time_long = in_time - time_;
  if ((work_time_ + l_time_long) >= work_limit_) {
    time_ += (work_limit_ - work_time_);
    work_time_ = work_limit_;
  } else {
    time_ = in_time;
    work_time_ += l_time_long;
  }
}
chrono::local_time_pos detail::work_next_clock_mfm::next_time(
    const chrono::milliseconds& in_du_time, const rules& in_rules) {
  auto l_day_1   = chrono::year_month_day{chrono::floor<chrono::days>(time_)};
  auto l_day_end = chrono::local_days{chrono::year_month_day_last{
                       l_day_1.year(),
                       chrono::month_day_last{l_day_1.month()}}} +
                   720h;
  work_limit_ = chrono::floor<chrono::seconds>(in_du_time);

  for (auto l_day = chrono::floor<chrono::days>(time_);
       l_day < l_day_end;
       l_day += chrono::days{1}) {
    auto l_r = in_rules(chrono::year_month_day{l_day});
    for (auto&& i : l_r) {
      i.add_event(*this);
      if (ok()) {
        return time_;
      }
    }
  }
  return time_;
}
void detail::work_next_clock_mfm::set_time(const chrono::local_time_pos& in_pos) {
  time_ = in_pos;
}
bool detail::work_next_clock_mfm::ok() const {
  return work_limit_ == work_time_;
}
}  // namespace business
namespace detail {

chrono::hours_double work_duration(const chrono::local_time_pos& in_s,
                                   const chrono::local_time_pos& in_e,
                                   const business::rules& in_rules) {
  const auto l_day_end = chrono::floor<chrono::days>(in_e);

  business::detail::work_clock_mfm l_mfm{};
  l_mfm.start();
  l_mfm.time_ = in_s;
  return l_mfm.work_duration(in_e, in_rules);
}
chrono::local_time_pos next_time(const chrono::local_time_pos& in_s,
                                 const chrono::milliseconds& in_du_time,
                                 const business::rules& in_rules) {
  business::detail::work_next_clock_mfm l_mfm{};
  l_mfm.start();
  l_mfm.set_time(in_s);
  return l_mfm.next_time(in_du_time, in_rules);
}
}  // namespace detail

}  // namespace doodle
