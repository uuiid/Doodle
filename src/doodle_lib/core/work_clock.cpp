//
// Created by TD on 2022/4/1.
//

#include "work_clock.h"
#include <date/tz.h>
#include <boost/contract.hpp>

namespace doodle {

namespace business {

std::optional<chrono::local_time_pos> rules::get_work_time(const chrono::local_time_pos& in_s) {
  //  auto l_time = chrono::make_zoned(chrono::current_zone(), in_s);
  /// \brief 首先查看在那天开始工作
  //  auto l_r = (*this)(chrono::year_month_day{chrono::floor<chrono::days>(in_s)});
  //  auto l_r_v =
  //      l_r |
  //      ranges::views::transform([](const decltype(l_r)::value_type& in_type) -> chrono::seconds {
  //        return in_type.first - in_type.second;
  //      }) |
  //      ranges::views::partial_sum([](const chrono::seconds& in_r, const chrono::seconds& in_l) {
  //        return in_r + in_l;
  //      }) |
  //      ranges::to_vector;

  //  return l_r.empty() ? {} : l_r.front();
}
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
  if (work_weekdays[l_weekday.c_encoding() - 1]) {
    ranges::for_each(work_pair, [&](const std::pair<chrono::seconds,
                                                    chrono::seconds>& in_pair) {
      time_list.emplace_back(l_local_days + in_pair.first, time_attr::work_begin);
      time_list.emplace_back(l_local_days + in_pair.second, time_attr::work_end);
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
  bool is_work = state_list.empty() ? false : state_list.back()[0];
  state_list.emplace_back(in_attr.state_);
  if (is_work)  /// 已经是工作状态
  {
    if (in_attr.state_ == work_attr::work_end)  /// \brief 传入工作结束
    {
      auto l_time_long = in_attr.time_point - time_point;
      if (work_limit_) {
        if ((work_time_ + l_time_long) > *work_limit_) {
          time_point += (*work_limit_ - l_time_long);
          work_time_ = *work_limit_;
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

  return *this;
}
bool work_clock::ok() const {
  if (work_limit_) {
    return *work_limit_ == work_time_;
  } else
    return !state_list.empty();
}
}  // namespace business
chrono::local_time_pos next_time(const chrono::local_time_pos& in_s,
                                 const std::chrono::milliseconds& in_du_time,
                                 const business::rules& in_rules) {
  auto l_day   = chrono::floor<chrono::days>(in_s);
  auto l_day_1 = chrono::year_month_day{l_day};
  chrono::year_month_day_last l_day_end{l_day_1.year(),
                                        chrono::month_day_last{l_day_1.month()}};
  std::chrono::milliseconds l_minutes{in_du_time};
  for (;
       l_day < chrono::local_days{l_day_end};
       l_day += chrono::days{1}) {
    auto l_r = in_rules(chrono::year_month_day{l_day});
    auto it  = ranges::find_if(l_r, [&](const decltype(l_r)::value_type& in_attr) -> bool {
      return (in_attr.state_ == decltype(l_r)::value_type::work_begin ||
              in_attr.state_ == decltype(l_r)::value_type::rest_end) &&
             in_attr.time_point < in_s;
    });
    if (it != l_r.end()) {
      return it->time_point;
    }
  }
}

}  // namespace doodle
