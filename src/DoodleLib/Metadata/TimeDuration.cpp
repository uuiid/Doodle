//
// Created by TD on 2021/5/17.
//

#include <DoodleLib/Metadata/TimeDuration.h>
#include <date/date.h>
namespace doodle {

TimeDuration::TimeDuration()
    : p_year(),
      p_month(),
      p_day(),
      p_hours(),
      p_minutes(),
      p_seconds() {
}
TimeDuration::TimeDuration(time_point in_point)
    : p_year(),
      p_month(),
      p_day(),
      p_hours(),
      p_minutes(),
      p_seconds() {
}
std::uint16_t TimeDuration::get_year() const {
  return (int)p_year;
}
void TimeDuration::set_year(std::uint16_t in_year) {
  p_year = date::year{in_year};
}
std::uint16_t TimeDuration::get_month() const {
  return (std::uint32_t)p_month;
}
void TimeDuration::set_month(std::uint16_t in_month) {
  p_month = date::month{in_month};
}
std::uint16_t TimeDuration::get_day() const {
  return (std::uint32_t)p_day;
}
void TimeDuration::set_day(std::uint16_t in_day) {
  p_day = date::day{in_day};
}
std::uint16_t TimeDuration::get_hour() const {
  return p_hours.count();
}
void TimeDuration::set_hour(std::uint16_t in_hour) {
  p_hours = std::chrono::hours{in_hour};
}
std::uint16_t TimeDuration::get_minutes() const {
  return p_minutes.count();
}
void TimeDuration::set_minutes(std::uint16_t in_minutes) {
  p_minutes = std::chrono::minutes{in_minutes};
}
std::uint16_t TimeDuration::get_second() const {
  return p_seconds.count();
}
void TimeDuration::set_second(std::uint16_t in_second) {
  p_seconds = std::chrono::seconds{in_second};
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
  date::weekday k_weekday{date::sys_days{p_year / p_month / p_day}};
  return k_weekday.c_encoding();
}

std::string TimeDuration::showStr() const {
  return date::format("%Y/%m/%d %H:%M", getTime());;
}
TimeDuration::time_point TimeDuration::getTime() const {
  return date::sys_days{p_year / p_month / p_day} + p_hours + p_minutes + p_seconds;
}

}  // namespace doodle
