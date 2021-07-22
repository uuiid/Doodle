//
// Created by TD on 2021/5/17.
//

#include <DoodleLib/Metadata/TimeDuration.h>
#include <date/date.h>
#include <date/tz.h>

namespace doodle {

TimeDuration::TimeDuration()
    : p_time(),
      p_year(),
      p_month(),
      p_day(),
      p_hours(),
      p_minutes(),
      p_seconds(),
      p_time_zone(date::current_zone()) {
}
TimeDuration::TimeDuration(time_point in_utc_timePoint)
    : p_time(),
      p_year(),
      p_month(),
      p_day(),
      p_hours(),
      p_minutes(),
      p_seconds(),
      p_time_zone(date::current_zone()) {
  disassemble(in_utc_timePoint);
}
std::uint16_t TimeDuration::get_year() const {
  return (int)p_year;
}
void TimeDuration::set_year(std::uint16_t in_year) {
  p_year = date::year{in_year};
  disassemble();
}
std::uint16_t TimeDuration::get_month() const {
  return (std::uint32_t)p_month;
}
void TimeDuration::set_month(std::uint16_t in_month) {
  p_month = date::month{in_month};
  disassemble();
}
std::uint16_t TimeDuration::get_day() const {
  return (std::uint32_t)p_day;
}
void TimeDuration::set_day(std::uint16_t in_day) {
  p_day = date::day{in_day};
  disassemble();
}
std::uint16_t TimeDuration::get_hour() const {
  return p_hours.count();
}
void TimeDuration::set_hour(std::uint16_t in_hour) {
  p_hours = std::chrono::hours{in_hour};
  disassemble();
}
std::uint16_t TimeDuration::get_minutes() const {
  return p_minutes.count();
}
void TimeDuration::set_minutes(std::uint16_t in_minutes) {
  p_minutes = std::chrono::minutes{in_minutes};
  disassemble();
}
std::uint16_t TimeDuration::get_second() const {
  return p_seconds.count();
}
void TimeDuration::set_second(std::uint16_t in_second) {
  p_seconds = std::chrono::seconds{in_second};
  disassemble();
}

template <>
std::string TimeDuration::getWeek() const {
  auto k_int = getWeek<std::int32_t>();
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

template <>
std::int32_t TimeDuration::getWeek() const {
  date::weekday k_weekday{date::local_days{p_year / p_month / p_day}};
  return k_weekday.c_encoding();
}

std::string TimeDuration::showStr() const {
  return date::format("%Y/%m/%d %H:%M:%S", getLocalTime());
}
TimeDuration::time_point TimeDuration::getUTCTime() const {
  return p_time;
}

void TimeDuration::disassemble() {
  auto k_time = date::local_days{p_year / p_month / p_day} + p_hours + p_minutes + p_seconds;
  auto k_      = date::make_zoned(p_time_zone, k_time);
  disassemble(k_.get_sys_time());
}
TimeDuration::time_point TimeDuration::getLocalTime() const {
  auto k_time = date::local_days{p_year / p_month / p_day} + p_hours + p_minutes + p_seconds;
  // auto k_time2 = p_time_zone->to_local(k_time);
  // time_point test = date::clock_cast<std::chrono::system_clock>(k_time);
  return date::clock_cast<std::chrono::system_clock>(k_time);
}

void TimeDuration::disassemble(const std::chrono::time_point<std::chrono::system_clock>& in_utc_timePoint) {
  // date::locate_zone("");
  // date::to_utc_time(p_time);
  p_time       = in_utc_timePoint;
  auto k_local = date::make_zoned(p_time_zone, in_utc_timePoint).get_local_time();

  auto k_dp = date::floor<date::days>(k_local);
  date::year_month_day k_day{k_dp};
  date::hh_mm_ss k_hh_mm_ss{date::floor<std::chrono::milliseconds>(k_local - k_dp)};
  p_year    = k_day.year();
  p_month   = k_day.month();
  p_day     = k_day.day();
  p_hours   = k_hh_mm_ss.hours();
  p_minutes = k_hh_mm_ss.minutes();
  p_seconds = k_hh_mm_ss.seconds();
}
TimeDuration::operator time_point() {
  return p_time;
}

}  // namespace doodle
