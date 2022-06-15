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
std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
rules::normal_works(const chrono::year_month_day& in_day) {
  std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>> l_r{};
  chick_true<doodle_error>(in_day.ok(), DOODLE_LOC, "无效的日期 {}", in_day);
  chrono::weekday l_weekday{in_day};
  /// \brief 加入工作日规定时间
  chrono::local_days l_local_days{in_day};
  if (work_weekdays[l_weekday.c_encoding()]) {
    ranges::for_each(work_pair, [&](const std::pair<chrono::seconds,
                                                    chrono::seconds>& in_pair) {
      l_r.emplace_back(std::make_pair(l_local_days + in_pair.first, l_local_days + in_pair.second));
    });
  }
  return l_r;
}
std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
rules::holidays(const chrono::year_month_day& in_day) {
  chrono::local_days l_local_days{in_day};
  /// 开始加入ji节假日
  auto l_r = extra_holidays |
             ranges::view::filter([&](const decltype(extra_work)::value_type& in_) -> bool {
               return doodle::chrono::floor<doodle::chrono::days>(in_.first) >= l_local_days &&
                      doodle::chrono::floor<doodle::chrono::days>(in_.second) <= l_local_days;
             }) |
             ranges::to_vector;

  return l_r;
}
std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
rules::adjusts(const chrono::year_month_day& in_day) {
  chrono::local_days l_local_days{in_day};
  /// 开始加入ji节假日
  auto l_r = extra_work |
             ranges::view::filter([&](const decltype(extra_work)::value_type& in_) -> bool {
               return doodle::chrono::floor<doodle::chrono::days>(in_.first) >= l_local_days &&
                      doodle::chrono::floor<doodle::chrono::days>(in_.second) <= l_local_days;
             }) |
             ranges::to_vector;

  return l_r;
}
std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>
rules::overtimes(const chrono::year_month_day& in_day) {
  chrono::local_days l_local_days{in_day};
  /// 开始加入ji节假日
  auto l_r = extra_rest |
             ranges::view::filter([&](const decltype(extra_work)::value_type& in_) -> bool {
               return doodle::chrono::floor<doodle::chrono::days>(in_.first) >= l_local_days &&
                      doodle::chrono::floor<doodle::chrono::days>(in_.second) <= l_local_days;
             }) |
             ranges::to_vector;

  return l_r;
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
}  // namespace business
namespace detail {
using time_pair     = std::pair<chrono::local_time_pos, chrono::local_time_pos>;
using time_list_v_t = std::vector<std::pair<chrono::local_time_pos, chrono::local_time_pos>>;
namespace {
bool sort_time(const time_pair& in_r, const time_pair& in_l) {
  return in_r.first < in_l.first;
}
bool is_time_overlap(const time_pair& in_a, const time_pair& in_b) {
  return (std::min(in_a.second, in_b.second) - std::max(in_a.first, in_b.first)) > time_pair::first_type::duration{0};
};
time_list_v_t time_du_add(const time_pair& in_a, const time_pair& in_b) {
  time_list_v_t l_r{};
  if (is_time_overlap(in_a, in_b)) {
    l_r.emplace_back(std::make_pair(std::min(in_a.first, in_b.first), std::max(in_a.second, in_b.second)));
  } else {
    l_r.emplace_back(in_a);
    l_r.emplace_back(in_b);
  }
  return l_r;
}
time_list_v_t time_du_sub(const time_pair& in_a, const time_pair& in_b) {
  time_list_v_t l_r{};
  if (in_a.first < in_b.first && in_a.second > in_b.second) {
    /**
     *  a1----------a2
     *    b1------b2
     */
    l_r.emplace_back(std::make_pair(in_a.first, in_b.first));
    l_r.emplace_back(std::make_pair(in_b.second, in_a.second));
  } else if (in_a.first > in_b.first && in_a.second < in_b.second) {
    /**
     *        a1-----a2
     *    b1--------------b2
     */
  } else if (in_a.first < in_b.first && in_a.second < in_b.second) {
    /**
     * a1-------------a2
     *        b1--------------b2
     */
    l_r.emplace_back(std::make_pair(in_a.first, in_b.first));
  } else if (in_a.first < in_b.first && in_a.second < in_b.second) {
    /**
     *            a1-------------a2
     *    b1--------------b2
     */
    l_r.emplace_back(std::make_pair(in_b.second, in_a.second));
  } else {
    /**
     *                            a1-------------a2
     *    b1--------------b2
     *
     *    or:
     *
     *    a1-------------a2
     *                        b1--------------b2
     */
    l_r.emplace_back(in_a);
  }
  return l_r;
};
}  // namespace

chrono::hours_double work_duration(const chrono::local_time_pos& in_s,
                                   const chrono::local_time_pos& in_e,
                                   const business::rules& in_rules) {
  const auto l_day_end = chrono::floor<chrono::days>(in_e);

  time_list_v_t l_normal_works{};
  time_list_v_t l_normal_rest{std::make_pair(in_s, in_e)};  /// \brief 初始我们传入时间全部休息
  time_list_v_t l_holidays{};
  time_list_v_t l_adjusts{};
  time_list_v_t l_overtimes{};
  time_list_v_t l_r{std::make_pair(in_s, in_e)};
  time_list_v_t l_r2{};

  /// \brief 计算休息时间
  while (!l_normal_works.empty()) {
    ranges::for_each(l_normal_rest, [&](const time_pair& in_p) {
      l_r2 |= ranges::actions::push_back(time_du_sub(in_p, l_normal_works.back()));
    });
    l_normal_rest |= ranges::actions::push_back(l_r2);
    l_r2.clear();
    l_normal_works.pop_back();
  }
  {
    l_r2.clear();
    /// \brief 减去休息时间
    while (!l_normal_rest.empty()) {
      ranges::for_each(l_r, [&](const time_pair& in_p) {
        l_r2 |= ranges::actions::push_back(time_du_sub(in_p, l_normal_rest.back()));
      });
      l_r |= ranges::actions::push_back(l_r2);
      l_r2.clear();
      l_normal_rest.pop_back();
    }
  }
  {
    l_r2.clear();
    /// \brief 减去节假日
    while (!l_holidays.empty()) {
      ranges::for_each(l_r, [&](const time_pair& in_p) {
        l_r2 |= ranges::actions::push_back(time_du_sub(in_p, l_holidays.back()));
      });
      l_r |= ranges::actions::push_back(l_r2);
      l_r2.clear();
      l_holidays.pop_back();
    }
  }
  {
    l_r2.clear();
    /// \brief 减去调休
    while (!l_adjusts.empty()) {
      ranges::for_each(l_r, [&](const time_pair& in_p) {
        l_r2 |= ranges::actions::push_back(time_du_sub(in_p, l_adjusts.back()));
      });
      l_r |= ranges::actions::push_back(l_r2);
      l_r2.clear();
      l_adjusts.pop_back();
    }
  }
  {
    l_r2.clear();
    /// \brief 加入加班
    while (!l_overtimes.empty()) {
      ranges::for_each(l_r, [&](const time_pair& in_p) {
        l_r2 |= ranges::actions::push_back(time_du_sub(in_p, l_overtimes.back()));
      });
      l_r |= ranges::actions::push_back(l_r2);
      l_r2.clear();
      l_overtimes.pop_back();
    }
  }
  auto l_i = time_pair ::first_type ::duration{0};
  ranges::for_each(l_r, [&](const time_pair& in) {
    l_i = in.second - in.first;
  });

  return l_i;
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
