//
// Created by TD on 2022/4/1.
//

#include "work_clock.h"
#include <date/tz.h>
#include <boost/contract.hpp>

#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/gregorian.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_set.hpp>

namespace doodle {

namespace business {

chrono::hours_double detail::work_clock_mfm::work_duration_(
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
chrono::local_time_pos detail::work_next_clock_mfm::next_time_(
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
void detail::work_machine_front::set_time_(const chrono::local_time_pos& in_pos) {
  time_ = in_pos;
}
bool detail::work_next_clock_mfm::ok() const {
  return work_limit_ == work_time_;
}

work_clock::work_clock() = default;

chrono::hours_double work_clock::operator()(
    const chrono::local_time_pos& in_min,
    const chrono::local_time_pos& in_max) const {
  auto l_d = discrete_interval_time::right_open(in_min,
                                                in_max);
  auto l_l = interval_set_time_ & l_d;
  chrono::hours_double l_len{};
  for (auto&& l_i : l_l) {
    l_len += boost::icl::last(l_i) - boost::icl::first(l_i);
  }
  return l_len;
}

chrono::local_time_pos work_clock::next_time(const chrono::local_time_pos& in_begin,
                                             const chrono::local_time_pos::duration& in_du) const {
  auto l_d = discrete_interval_time::right_open(in_begin, in_begin.max());
  auto l_l = interval_set_time_ & l_d;
  chrono::hours_double l_len{};
  for (auto&& l_i : l_l) {
    auto l_en_t = boost::icl::last(l_i) - boost::icl::first(l_i);
    if ((l_en_t + l_len) > in_du) {
      return boost::icl::first(l_i) +
             doodle::chrono::floor<doodle::chrono::seconds>(in_du - l_len);
    } else {
      l_len += l_en_t;
    }
  }

  return {};
}

void work_clock::gen_rules_(const discrete_interval_time& in_time) {
  auto l_begin = doodle::chrono::floor<doodle::chrono::days>(boost::icl::first(in_time));
  auto l_end   = doodle::chrono::floor<doodle::chrono::days>(boost::icl::last(in_time));
  interval_set_time l_r;
  for (;
       l_begin <= l_end;
       l_begin += chrono::days{1}) {
    /// \brief 加入工作日规定时间
    chrono::local_days l_local_days{l_begin};
    chrono::weekday l_weekday{l_begin};
    if (rules_.work_weekdays[l_weekday.c_encoding()]) {
      ranges::for_each(rules_.work_pair, [&](const std::pair<chrono::seconds,
                                                             chrono::seconds>& in_pair) {
        l_r += discrete_interval_time::right_open(l_begin + in_pair.first,
                                                  l_begin + in_pair.second);
      });
    }
  }
  /// \brief 减去节假日
  ranges::for_each(
      rules_.extra_holidays,
      [&](const decltype(rules_.extra_holidays)::value_type& in_) {
        l_r -= discrete_interval_time::right_open(in_.first, in_.second);
      });
  /// \brief 减去调休
  ranges::for_each(
      rules_.extra_rest,
      [&](const decltype(rules_.extra_rest)::value_type& in_) {
        l_r -= discrete_interval_time::right_open(in_.first, in_.second);
      });
  /// \brief 加上加班
  ranges::for_each(
      rules_.extra_work,
      [&](const decltype(rules_.extra_work)::value_type& in_) {
        l_r += discrete_interval_time::right_open(in_.first, in_.second);
      });
  interval_set_time_ = l_r;
}
void work_clock::set_rules(const rules& in_rules) {
  rules_ = in_rules;
}
void work_clock::set_interval(const chrono::local_time_pos& in_min,
                              const chrono::local_time_pos& in_max) {
  gen_rules_(discrete_interval_time::right_open(in_min,
                                                in_max));
}

}  // namespace business
namespace detail {

chrono::hours_double work_duration(const chrono::local_time_pos& in_s,
                                   const chrono::local_time_pos& in_e,
                                   const business::rules& in_rules) {
  business::work_clock l_c{};
  l_c.set_rules(in_rules);
  l_c.set_interval(in_s, in_e);
  return l_c(in_s, in_e);
}
chrono::local_time_pos next_time(const chrono::local_time_pos& in_s,
                                 const chrono::local_time_pos::duration& in_du_time,
                                 const business::rules& in_rules) {
  business::work_clock l_c{};
  l_c.set_rules(in_rules);
  l_c.set_interval(in_s, in_s + doodle::chrono::days{33});
  return l_c.next_time(in_s,
                       doodle::chrono::floor<chrono::local_time_pos::duration>(in_du_time));
}
}  // namespace detail

}  // namespace doodle
