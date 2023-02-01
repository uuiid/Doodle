//
// Created by TD on 2022/4/1.
//

#include "work_clock.h"

#include <doodle_core/lib_warp/boost_fmt_icl.h>
#include <doodle_core/lib_warp/std_fmt_set.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/detail/time_point_info.h>

#include <boost/contract.hpp>
#include <boost/icl/concept/interval.hpp>
#include <boost/icl/discrete_interval.hpp>
#include <boost/icl/gregorian.hpp>
#include <boost/icl/interval_bounds.hpp>
#include <boost/icl/interval_map.hpp>
#include <boost/icl/split_interval_set.hpp>

#include "logger/logger.h"
#include "metadata/time_point_wrap.h"
#include "time_tool/work_clock.h"
#include <chrono>
#include <date/tz.h>
#include <range/v3/range.hpp>

namespace doodle::business {

work_clock::work_clock() = default;

void work_clock::cut_interval(const time_type& in_min, const time_type& in_max) {
  auto l_d = discrete_interval_time::closed(in_min, in_max);
  interval_set_time_ &= l_d;
  interval_map_time_ &= l_d;
}

work_clock::duration_type work_clock::operator()(const time_type& in_min, const time_type& in_max) const {
  auto l_d = discrete_interval_time::closed(in_min, in_max);
  auto l_l = interval_set_time_ & l_d;
  duration_type l_len{};
  for (const auto& l_i : l_l) {
    l_len += (l_i.upper() - l_i.lower());
    // l_len += (boost::icl::upper(l_i).get_sys_time() - boost::icl::lower(l_i).get_sys_time());
    // DOODLE_LOG_INFO("{} - {} = {}", l_i.upper(), l_i.lower(), chrono::round<chrono::seconds>(l_len));
  }
  return l_len;
}

work_clock::time_type work_clock::next_time(const time_type& in_begin, const duration_type& in_du) const {
  auto l_d = discrete_interval_time::right_open(in_begin, time_type::max());
  auto l_l = interval_set_time_ & l_d;
  duration_type l_len{};
  for (auto&& l_i : l_l) {
    auto l_en_t = boost::icl::upper(l_i) - boost::icl::lower(l_i);
    if ((l_en_t + l_len) >= in_du) {
      return boost::icl::first(l_i) + doodle::chrono::round<doodle::chrono::seconds>(in_du - l_len);
    } else {
      l_len += l_en_t;
    }
  }

  return {};
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
std::optional<std::string> work_clock::get_time_info(const time_type& in_min, const time_type& in_max) const {
  auto l_d = discrete_interval_time::closed(in_min, in_max);
  // auto l_item                          = interval_map_time_ & l_d;

  std::string l_r{};
  for (auto&& i : interval_map_time_) {
    auto l_t1 = i.first & l_d;
    if (!boost::icl::is_empty(l_t1))
      l_r += fmt::format(
          R"("{:L%Y-%m-%d %H:%M} 到 {:L%Y-%m-%d %H:%M}  信息 {}")", ++boost::icl::lower(i.first),
          boost::icl::upper(i.first), fmt::join(i.second, " ")
      );
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
  // DOODLE_LOG_INFO("减去时间 {} -> {} {}", l_time_1, l_time_2, l_info);
  // DOODLE_LOG_INFO("减去时间2 {} ", l_dis);
  interval_set_time_ -= l_dis;
  interval_map_time_ += std::make_pair(discrete_interval_time::right_open(l_time_1, l_time_2), info_type{l_info});
  // DOODLE_LOG_INFO("时间段 {}", interval_set_time_);
  return *this;
}
void work_clock::add_info(const std::tuple<time_point_wrap, time_point_wrap, std::string>& in_time) {
  auto&& [l_time_1, l_time_2, l_info] = in_time;
  auto l_dis                          = discrete_interval_time::closed(l_time_1, l_time_2);
  interval_map_time_ += std::make_pair(discrete_interval_time::right_open(l_time_1, l_time_2), info_type{l_info});
}

work_clock::time_type work_clock::next_point(const work_clock::time_type& in_point) {
  auto l_d = discrete_interval_time::right_open(in_point, time_type::max());
  auto l_l = interval_set_time_ & l_d;
  for (auto&& l_i : l_l) {
    return boost::icl::lower(l_i);
  }

  return {};
}
}  // namespace doodle::business
