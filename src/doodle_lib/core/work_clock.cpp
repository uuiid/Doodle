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
void work_clock::set_work_limit(
    const chrono::local_time_pos& in_pos,
    const chrono::seconds& in_work_du) {
  work_limit_ = in_work_du;
  time_point  = in_pos;
}
chrono::seconds work_clock::work_time() const {
  return work_time_;
}
work_clock& work_clock::operator+=(const time_attr& in_attr) {
  /// \brief 优先检查时间点是否比时钟时间点早
  if (in_attr.time_point < time_point) {
    state_list.emplace_back(in_attr.state_);
    return *this;
  }

  auto up_state = state_list.empty() ? work_attr::normal_work_end : state_list.back();
  if (up_state == work_attr::normal_work_end ||
      up_state == work_attr::adjust_rest_end ||
      up_state == work_attr::adjust_work_begin)  /// 开始进入工作
  {
    if (in_attr.state_ == work_attr::normal_work_begin)
      time_point = in_attr.time_point;
    //    if (up_state == work_attr::adjust_rest_end)  /// 调整结束状态要查看前面几个状态
    //    {
    //
    //    }
  } else if (up_state == work_attr::normal_work_begin ||
             up_state == work_attr::adjust_work_begin)  /// \brief 可以结束工作计算时间
  {
    if (in_attr.state_ == work_attr::normal_work_end ||
        in_attr.state_ == work_attr::adjust_work_end ||
        in_attr.state_ == work_attr::adjust_rest_begin)  /// \brief 传入工作结束
    {
      auto l_time_long = in_attr.time_point - time_point;
      if (work_limit_) {
        if ((work_time_ + l_time_long) > *work_limit_) {
          time_point += (*work_limit_ - work_time_);
          work_time_ = *work_limit_;
        } else {
          time_point = in_attr.time_point;
          work_time_ += l_time_long;
        }
      } else {
        time_point = in_attr.time_point;
        work_time_ += l_time_long;
      }
    }
  }
  //  else  /// \brief 已经是休息状态
  //  {
  //    if (in_attr.state_ == work_attr::work_begin)  /// \brief 传入工作开始
  //    {
  //    }
  //  }

  state_list.emplace_back(in_attr.state_);
  return *this;
}
bool work_clock::ok() const {
  if (work_limit_) {
    return *work_limit_ == work_time_;
  } else
    return !state_list.empty();
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

  business::work_clock l_clock{in_s};
  l_clock.set_work_limit(in_s, chrono::floor<chrono::seconds>(in_du_time));
  for (auto l_day = chrono::floor<chrono::days>(in_s);
       l_day < l_day_end;
       l_day += chrono::days{1}) {
    auto l_r = in_rules(chrono::year_month_day{l_day});
    for (auto&& i : l_r) {
      l_clock += i;
      if (l_clock) {
        return l_clock.time_point;
      }
    }
  }
  return l_clock.time_point;
}
chrono::hours_double work_duration(const chrono::local_time_pos& in_s,
                                   const chrono::local_time_pos& in_e,
                                   const business::rules& in_rules) {
  business::work_clock l_clock{in_s};
  const auto l_day_end = chrono::floor<chrono::days>(in_e);

  for (auto l_day = chrono::floor<chrono::days>(in_s);
       l_day <= l_day_end;
       l_day += chrono::days{1}) {
    auto l_r = in_rules(chrono::year_month_day{l_day});
    for (auto&& i : l_r) {
      if (i.time_point <= in_e) {
        l_clock += i;
      } else {
        l_clock += business::time_attr{in_e, i.state_};
        return l_clock.work_time();
      }
    }
  }
  return l_clock.work_time();
}
}  // namespace detail

}  // namespace doodle
