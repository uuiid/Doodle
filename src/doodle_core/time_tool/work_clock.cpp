//
// Created by TD on 2022/4/1.
//

#include "work_clock.h"

#include <doodle_core/lib_warp/boost_fmt_icl.h>
#include <doodle_core/lib_warp/std_fmt_set.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/detail/time_point_info.h>

#include <boost/contract.hpp>
#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/gregorian.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_set.hpp>

#include <date/tz.h>
#include <range/v3/range.hpp>

namespace doodle {

namespace business {

work_clock::work_clock() = default;

work_clock::duration_type work_clock::operator()(const time_type& in_min, const time_type& in_max) const {
  auto l_d = discrete_interval_time::closed(in_min, in_max);
  auto l_l = interval_set_time_ & l_d;
  duration_type l_len{};
  for (const auto& l_i : l_l) {
    l_len += (l_i.upper() - l_i.lower());
    //    l_len += (boost::icl::upper(l_i).get_sys_time() - boost::icl::lower(l_i).get_sys_time());
    //    DOODLE_LOG_INFO("{} -> {} = {}", l_i.upper(), l_i.lower(), chrono::floor<chrono::seconds>(l_len));
  }
  return l_len;
}

work_clock::time_type work_clock::next_time(const time_type& in_begin, const duration_type& in_du) const {
  auto l_d = discrete_interval_time::right_open(in_begin, time_type::max());
  auto l_l = interval_set_time_ & l_d;
  duration_type l_len{};
  for (auto&& l_i : l_l) {
    auto l_en_t = boost::icl::upper(l_i) - boost::icl::lower(l_i);
    if ((l_en_t + l_len) > in_du) {
      return boost::icl::first(l_i) + doodle::chrono::floor<doodle::chrono::seconds>(in_du - l_len);
    } else {
      l_len += l_en_t;
    }
  }

  return {};
}

void work_clock::gen_rules_(const discrete_interval_time& in_time) {
  /// \brief 如果已经包含就直接返回
  //  if (!boost::icl::contains(boost::icl::hull(interval_set_time_), in_time)) return;

  auto l_begin = boost::icl::first(in_time);
  auto l_end   = boost::icl::last(in_time);

  interval_set_time l_r;
  for (; l_begin <= l_end; l_begin += chrono::days{1}) {
    /// \brief 加入工作日规定时间
    if (rules_.work_weekdays()[l_begin.get_week_int()]) {
      ranges::for_each(rules_.work_time(), [&](const std::pair<chrono::seconds, chrono::seconds>& in_pair) {
        l_r += discrete_interval_time::closed(l_begin + in_pair.first, l_begin + in_pair.second);
      });
    }
  }
  /// \brief 减去节假日
  ranges::for_each(
      rules_.extra_holidays(), [&](const std::decay_t<decltype(rules_.extra_holidays())>::value_type& in_
                               ) { l_r -= discrete_interval_time::open(in_.first, in_.second); }
  );
  /// \brief 减去调休
  ranges::for_each(rules_.extra_rest(), [&](const std::decay_t<decltype(rules_.extra_rest())>::value_type& in_) {
    l_r -= discrete_interval_time::open(in_.first, in_.second);
  });
  /// \brief 加上加班
  ranges::for_each(rules_.extra_work(), [&](const std::decay_t<decltype(rules_.extra_work())>::value_type& in_) {
    l_r += discrete_interval_time::closed(in_.first, in_.second);
  });
  interval_set_time_ = l_r;
}

void work_clock::generate_interval_map_time_(const discrete_interval_time& in_time) {
  //  auto l_begin = boost::icl::first(in_time);
  //  auto l_end   = boost::icl::last(in_time);

  interval_map_time l_r;

  /// \brief 减去节假日
  ranges::for_each(
      rules_.extra_holidays(),
      [&](const std::decay_t<decltype(rules_.extra_holidays())>::value_type& in_) {
        l_r += std::make_pair(discrete_interval_time::closed(in_.first, in_.second), info_type{in_.info});
      }
  );
  /// \brief 减去调休
  ranges::for_each(rules_.extra_rest(), [&](const std::decay_t<decltype(rules_.extra_rest())>::value_type& in_) {
    l_r += std::make_pair(discrete_interval_time::closed(in_.first, in_.second), info_type{in_.info});
  });
  /// \brief 加上加班
  ranges::for_each(rules_.extra_work(), [&](const std::decay_t<decltype(rules_.extra_work())>::value_type& in_) {
    l_r += std::make_pair(discrete_interval_time::closed(in_.first, in_.second), info_type{in_.info});
  });
  interval_map_time_ = l_r;
}

void work_clock::set_rules(const rules& in_rules) {
  rules_ = in_rules;
  interval_set_time_.clear();
  interval_map_time_.clear();
}
void work_clock::set_interval(const time_type& in_min, const time_type& in_max) {
  auto l_item = discrete_interval_time::closed(in_min, in_max);
  gen_rules_(l_item);
  generate_interval_map_time_(l_item);
}

std::vector<std::pair<work_clock::time_type, work_clock::time_type>> work_clock::get_work_du(
    const time_type& in_min, const time_type& in_max
) const {
  std::vector<std::pair<time_type, time_type>> l_r{};
  auto l_d = discrete_interval_time::closed(in_min, in_max);
  auto l_l = interval_set_time_ & l_d;
  for (auto&& l_i : l_l) {
    l_r.emplace_back(boost::icl::first(l_i), boost::icl::last(l_i));
  }
  return l_r;
}

std::string work_clock::debug_print() const {
  return fmt::format("时间段 {}  时间信息 {}", interval_set_time_, interval_map_time_);

  //  for (auto&& i : interval_map_time_) {
  //    fmt::format("{} ", i.second);
  //    auto l_item = i.first;
  //    fmt::format("{} ", i.first);
  //  }
  //
  //  return fmt::format("{}", interval_map_time_);
}
std::optional<std::string> work_clock::get_time_info(const time_type& in_min, const time_type& in_max) {
  auto l_d    = discrete_interval_time::closed(in_min, in_max);

  auto l_item = interval_map_time_ & l_d;

  std::string l_r{};
  for (auto&& i : l_item) {
    l_r += fmt::to_string(fmt::join(i.second, " "));
  }

  return l_r.empty() ? std::optional<std::string>{} : std::optional{l_r};
}

work_clock& work_clock::operator+=(const std::tuple<time_point_wrap, time_point_wrap>& in_time) {
  auto&& [l_time_1, l_time_2] = in_time;
  auto l_dis                  = discrete_interval_time::closed(l_time_1, l_time_2);
  interval_set_time_ += l_dis;
  return *this;
}

work_clock& work_clock::operator-=(const std::tuple<time_point_wrap, time_point_wrap>& in_time) {
  auto&& [l_time_1, l_time_2] = in_time;
  auto l_dis                  = discrete_interval_time::closed(l_time_1, l_time_2);
  interval_set_time_ -= l_dis;

  return *this;
}

work_clock& work_clock::operator+=(const std::tuple<time_point_wrap, time_point_wrap, std::string>& in_time) {
  auto&& [l_time_1, l_time_2, l_info] = in_time;
  auto l_dis                          = discrete_interval_time::closed(l_time_1, l_time_2);
  interval_set_time_ += l_dis;
  interval_map_time_ += std::make_pair(discrete_interval_time::right_open(l_time_1, l_time_2), info_type{l_info});
  return *this;
}

work_clock& work_clock::operator-=(const std::tuple<time_point_wrap, time_point_wrap, std::string>& in_time) {
  auto&& [l_time_1, l_time_2, l_info] = in_time;
  auto l_dis                          = discrete_interval_time::closed(l_time_1, l_time_2);
  interval_set_time_ -= l_dis;
  interval_map_time_ += std::make_pair(discrete_interval_time::right_open(l_time_1, l_time_2), info_type{l_info});

  return *this;
}
}  // namespace business

}  // namespace doodle
