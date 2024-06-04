﻿//
// Created by TD on 2021/5/17.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/operators.hpp>
#include <boost/pfr.hpp>
#include <boost/pfr/functions_for.hpp>

#include <chrono>
#include <fmt/chrono.h>

namespace doodle {
class time_point_wrap;

template <typename TimePoint_T>
auto parse_8601(const std::string& in_str, boost::system::error_code& ec) {
  TimePoint_T l_time_point;
  std::istringstream l_stream{in_str};
  l_stream >> std::chrono::parse("%FT%T", l_time_point);
  // if (in_str.back() == 'Z') {
  // } else {
  //   l_stream >> std::chrono::parse("%FT%T%Ez", l_time_point);
  // }
  if (!l_stream) {
    ec = boost::system::error_code{ERROR_CLUSTER_INVALID_STRING_FORMAT, boost::system::system_category()};
  }
  return l_time_point;
}
template <typename TimePoint_T>
auto parse_8601(const std::string& in_str) {
  boost::system::error_code l_ec;
  auto l_time_point = parse_8601<TimePoint_T>(in_str, l_ec);
  if (l_ec) {
    throw std::runtime_error(l_ec.message());
  }
  return l_time_point;
}

namespace chrono_ns {
using namespace std::chrono;
namespace detail {
minutes get_local_bias();
}

template <typename Duration_T>
time_point<chrono::local_t, Duration_T> to_local_point(const time_point<system_clock, Duration_T>& in) {
  return time_point<chrono::local_t, Duration_T>{
      std::chrono::round<Duration_T>(in - detail::get_local_bias()).time_since_epoch()
  };
}

template <typename Duration_T>
time_point<system_clock, Duration_T> to_sys_point(const time_point<chrono::local_t, Duration_T>& in) {
  return time_point<system_clock, Duration_T>{
      std::chrono::round<Duration_T>(in + detail::get_local_bias()).time_since_epoch()
  };
}
}  // namespace chrono_ns

namespace time_point_wrap_ns {
using time_point       = chrono::sys_time_pos;
using time_duration    = time_point::duration;
using duration         = time_point::duration;
using time_local_point = chrono::local_time<time_duration>;
class compose_type {
 public:
  std::uint16_t year;
  std::uint16_t month;
  std::uint16_t day;
  std::uint16_t hours;
  std::uint16_t minutes;
  std::uint16_t seconds;
};
BOOST_PFR_FUNCTIONS_FOR(compose_type);

class compose_2_type {
 public:
  chrono::local_days y_m_d;
  chrono::seconds h_m_s;
};

}  // namespace time_point_wrap_ns

/**
 * @brief 这是一个小的时间类
 * @warning 这个类中的设置时间的函数和都是设置本地日期的，并不是utc时间， 他会自动在内部转换为utc
 */
class DOODLE_CORE_API time_point_wrap : boost::totally_ordered<time_point_wrap>,
                                        boost::additive<time_point_wrap, time_point_wrap_ns::duration>,
                                        boost::unit_steppable<time_point_wrap> {
 public:
  using time_point       = chrono::sys_time_pos;
  using time_duration    = time_point::duration;
  using duration         = time_point::duration;
  using time_local_point = chrono::local_time<time_duration>;
  using compose_type     = time_point_wrap_ns::compose_type;
  using compose_2_type   = time_point_wrap_ns::compose_2_type;

 private:
  void set_time(const time_local_point& in);
  void set_time(const time_point& in);

  time_point sys_point{};

 public:
  time_point_wrap();
  virtual ~time_point_wrap();
  template <class Clock, class Duration>
  explicit time_point_wrap(const chrono::time_point<Clock, Duration>& in_time_point) {
    set_time(chrono::round<time_point_wrap::time_point::duration>(in_time_point));
  }

  explicit time_point_wrap(
      std::int32_t in_year, std::int32_t in_month, std::int32_t in_day, std::int32_t in_hours = 0,
      std::int32_t in_minutes = 0, std::int32_t in_seconds = 0
  );

  /**
   * @brief 分解函数
   * @warning 返回时间均为本地时间
   * @return 返回本地时间段分解(年月日时分秒)
   */
  [[nodiscard]] compose_type  // seconds
  compose() const;
  /**
   * @brief 简单段分解函数
   * @warning 本地时间段分解
   * @return 转换为天数和秒数
   */
  [[nodiscard("")]] compose_2_type compose_1() const;

  /**
   * @brief 本地时间转换段周索引
   * @warning 此处返回本地时间段周索引
   * @return
   */
  [[nodiscard]] std::int32_t get_week_int() const;

  static time_point_wrap current_month_end(const time_point_wrap& in_time);
  static time_point_wrap current_month_start(const time_point_wrap& in_time);
  /**
   * @brief 当月段结束
   * @return
   */
  [[nodiscard]] time_point_wrap current_month_end() const;
  /**
   * @brief 当月的开开始
   * @return
   */
  [[nodiscard]] time_point_wrap current_month_start() const;

  /**
   * @brief 最小时间
   * @return
   */
  static time_point_wrap min();
  /**
   * @brief 最大时间
   * @return
   */
  static time_point_wrap max();

  [[nodiscard("")]] time_point get_sys_time() const;
  [[nodiscard("")]] time_local_point get_local_time() const;

  /**
   * @brief 现在时间
   * @return
   */
  static time_point_wrap now();

  bool operator==(const time_point_wrap& in_rhs) const;
  bool operator<(const time_point_wrap& in_rhs) const;
  time_point_wrap& operator+=(const duration& in_dur);
  time_point_wrap& operator-=(const duration& in_dur);

  time_point_wrap& operator++();
  time_point_wrap& operator--();

  template <typename Rep_T, typename Period_T>
  time_point_wrap operator+(const doodle::chrono::duration<Rep_T, Period_T>& in_dur) {
    auto l_sys_time = sys_point;
    l_sys_time += in_dur;
    return time_point_wrap{l_sys_time};
  }

  template <typename Rep_T, typename Period_T>
  time_point_wrap operator-(const doodle::chrono::duration<Rep_T, Period_T>& in_dur) {
    auto l_sys_time = sys_point;
    l_sys_time -= in_dur;
    return time_point_wrap{l_sys_time};
  }

  template <class Clock, class Duration = typename Clock::duration>
  time_point_wrap& operator=(const doodle::chrono::time_point<Clock, Duration>& in_dur) {
    this->set_time(chrono::floor<duration>(in_dur));
    return *this;
  }

  [[nodiscard("not ")]] explicit operator std::tm() const;

 private:
  // 这里是序列化的代码
  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const time_point_wrap& p);
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, time_point_wrap& p);
};

inline auto format_as(time_point_wrap f) { return f.get_sys_time(); }

time_point_wrap::duration DOODLE_CORE_API operator-(const time_point_wrap& in_l, const time_point_wrap& in_r);

}  // namespace doodle

namespace doodle {
template <typename CharType, typename CharTraits>
std::basic_ostream<CharType, CharTraits>& operator<<(
    std::basic_ostream<CharType, CharTraits>& stream, const time_point_wrap& in_point_wrap
) {
  return stream << fmt::to_string(in_point_wrap);
}
}  // namespace doodle

#include "doodle_core/lib_warp/chrono_fmt.h"