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
  /// \brief 如果已经包含就直接返回
  //  if (!boost::icl::contains(boost::icl::hull(interval_set_time_), in_time)) return;

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
  interval_set_time_.clear();
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
