﻿//
// Created by TD on 2021/5/17.
//

#include <doodle_core/metadata/metadata.h>
#include <date/date.h>
#include <date/tz.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/logger/logger.h>

namespace doodle {
void to_json(nlohmann::json& j, const time_point_wrap& p) {
  j["time"] = p.zoned_time_.get_sys_time();
}
void from_json(const nlohmann::json& j, time_point_wrap& p) {
  p.zoned_time_ = j.at("time").get<time_point_wrap::time_point>();
}
time_point_wrap::time_point_wrap()
    : time_point_wrap(time_point::clock::now()) {
}
time_point_wrap::time_point_wrap(const time_point_wrap::time_zoned& in_time_zoned)
    : zoned_time_(in_time_zoned) {
}
time_point_wrap::time_point_wrap(time_point in_utc_timePoint)
    : time_point_wrap(chrono::make_zoned(date::current_zone(), in_utc_timePoint)) {
}

time_point_wrap::time_point_wrap(time_local_point in_local_time_point)
    : time_point_wrap(chrono::make_zoned(date::current_zone(), in_local_time_point)) {
}

time_point_wrap::time_point_wrap(
    std::int32_t in_year,
    std::int32_t in_month,
    std::int32_t in_day,
    std::int32_t in_hours,
    std::int32_t in_minutes,
    std::int32_t in_seconds)
    : time_point_wrap(chrono::local_time_pos{chrono::local_days{chrono::year{boost::numeric_cast<std::int32_t>(in_year)} /
                                                                chrono::month{boost::numeric_cast<std::uint32_t>(in_month)} /
                                                                chrono::day{boost::numeric_cast<std::uint32_t>(in_day)}} +
                                             chrono::hours{in_hours} +
                                             chrono::minutes{in_minutes} +
                                             chrono::seconds{in_seconds}}) {
}

std::string time_point_wrap::get_week_s() const {
  auto k_int = get_week_int();
  std::string k_string{};
  switch (k_int) {
    case 0:
      k_string = "星期日";
      break;
    case 1:
      k_string = "星期一";
      break;
    case 2:
      k_string = "星期二";
      break;
    case 3:
      k_string = "星期三";
      break;
    case 4:
      k_string = "星期四";
      break;
    case 5:
      k_string = "星期五";
      break;
    case 6:
      k_string = "星期六";
      break;
    default:
      k_string = "未知";
      break;
  }
  return k_string;
}

std::int32_t time_point_wrap::get_week_int() const {
  date::weekday k_weekday{chrono::time_point_cast<date::days>(zoned_time_.get_local_time())};
  return k_weekday.c_encoding();
}

std::string time_point_wrap::show_str() const {
  return date::format("%Y/%m/%d %H:%M:%S", zoned_time_.get_local_time());
}

std::tuple<std::uint16_t,  // year
           std::uint16_t,  // month
           std::uint16_t,  // day
           std::uint16_t,  // hours
           std::uint16_t,  // minutes
           std::uint16_t>
time_point_wrap::compose() const {
  auto k_local = zoned_time_.get_local_time();
  auto k_dp    = date::floor<date::days>(k_local);
  date::year_month_day k_day{k_dp};
  date::hh_mm_ss k_hh_mm_ss{date::floor<std::chrono::milliseconds>(k_local - k_dp)};
  return std::make_tuple(boost::numeric_cast<std::uint16_t>((std::int32_t)k_day.year()),
                         boost::numeric_cast<std::uint16_t>((std::uint32_t)k_day.month()),
                         boost::numeric_cast<std::uint16_t>((std::uint32_t)k_day.day()),
                         boost::numeric_cast<std::uint16_t>(k_hh_mm_ss.hours().count()),
                         boost::numeric_cast<std::uint16_t>(k_hh_mm_ss.minutes().count()),
                         boost::numeric_cast<std::uint16_t>(k_hh_mm_ss.seconds().count()));
}

bool time_point_wrap::operator==(const time_point_wrap& in_rhs) const {
  return zoned_time_ == in_rhs.zoned_time_;
}
bool time_point_wrap::operator<(const time_point_wrap& in_rhs) const {
  return zoned_time_.get_sys_time() < in_rhs.zoned_time_.get_sys_time();
}

bool operator<(const time_point_wrap& in_l, const time_point_wrap::time_point& in_r) {
  return in_l.zoned_time_.get_sys_time() < in_r;
};
bool operator<(const time_point_wrap& in_l, const time_point_wrap::time_local_point& in_r) {
  return in_l.zoned_time_.get_local_time() < in_r;
};
bool operator<(const time_point_wrap& in_l, const time_point_wrap::time_zoned& in_r) {
  return in_l.zoned_time_.get_sys_time() < in_r.get_sys_time();
};
bool operator<(const time_point_wrap::time_point& in_l, const time_point_wrap& in_r) {
  return in_l < in_r.zoned_time_.get_sys_time();
};
bool operator<(const time_point_wrap::time_local_point& in_l, const time_point_wrap& in_r) {
  return in_l < in_r.zoned_time_.get_local_time();
};
bool operator<(const time_point_wrap::time_zoned& in_l, const time_point_wrap& in_r) {
  return in_l.get_sys_time() < in_r.zoned_time_.get_sys_time();
};


bool time_point_wrap::operator<(const time_point_wrap::time_point& in_l) {
  return this->zoned_time_.get_sys_time() < in_l ;
};
bool time_point_wrap::operator<(const time_point_wrap::time_local_point& in_l) {
  return this->zoned_time_.get_local_time() < in_l ;
};
bool time_point_wrap::operator<(const time_point_wrap::time_zoned& in_l) {
  return this->zoned_time_.get_sys_time()  < in_l.get_sys_time();
};


time_point_wrap time_point_wrap::current_month_end(const time_point_wrap& in_time) {
  auto&& [l_y, l_m, l_d, l_1, l_2, l_3] = in_time.compose();
  auto l_mo                             = chrono::year{l_y} / chrono::month{l_m} / chrono::last;
  return time_point_wrap{chrono::local_days{l_mo} + 23h + 59min + 59s};
}
time_point_wrap time_point_wrap::min() {
  auto l_p = time_point_wrap{time_point::min()} > time_point ::clock ::now();
  auto l_p2 = time_point_wrap{time_point::min()} > time_point_wrap{time_point::min()};

  return time_point_wrap{time_point::min()};
}
time_point_wrap time_point_wrap::max() {
  return time_point_wrap{time_point::max()};
}
time_point_wrap time_point_wrap::current_month_start(const time_point_wrap& in_time) {
  auto&& [l_y, l_m, l_d, l_1, l_2, l_3] = in_time.compose();
  auto l_mo                             = chrono::year{l_y} / chrono::month{l_m - 1u} / chrono::last;
  return time_point_wrap{chrono::local_days{l_mo} + doodle::chrono::days{1}};
}
time_point_wrap time_point_wrap::current_month_end() const {
  return current_month_end(*this);
}
time_point_wrap time_point_wrap::current_month_start() const {
  return current_month_start(*this);
}
void time_point_wrap::set_time(const time_point_wrap::time_local_point& in) {
  zoned_time_ = chrono::make_zoned(zoned_time_.get_time_zone(), in);
}
void time_point_wrap::set_time(const time_point_wrap::time_point& in) {
  zoned_time_ = chrono::make_zoned(zoned_time_.get_time_zone(), in);
}
void time_point_wrap::set_time(const time_zoned& in) {
  zoned_time_ = in;
}
}  // namespace doodle
