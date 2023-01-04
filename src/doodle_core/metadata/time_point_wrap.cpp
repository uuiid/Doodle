//
// Created by TD on 2021/5/17.
//

#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <Windows.h>
#include <chrono>
#include <date/date.h>
#include <date/tz.h>
// #include <timeapi.h>
// #include <timezoneapi.h>

namespace doodle {
namespace {
time_point_wrap::time_local_point to_local_point(const time_point_wrap::time_point& in) {
  TIME_ZONE_INFORMATION L_time;
  auto l_err = GetTimeZoneInformation(&L_time);
  if (l_err != TIME_ZONE_ID_INVALID) {
    chrono::minutes l_m{L_time.Bias};
    return time_point_wrap::time_local_point{(in - l_m).time_since_epoch()};
  }
  return time_point_wrap::time_local_point{in.time_since_epoch()};
}

time_point_wrap::time_point to_sys_point(const time_point_wrap::time_local_point& in) {
  TIME_ZONE_INFORMATION L_time;
  auto l_err = GetTimeZoneInformation(&L_time);
  if (l_err != TIME_ZONE_ID_INVALID) {
    chrono::minutes l_m{L_time.Bias};
    return time_point_wrap::time_point{(in + l_m).time_since_epoch()};
  }
  return time_point_wrap::time_point{in.time_since_epoch()};
}

}  // namespace

void to_json(nlohmann::json& j, const time_point_wrap& p) { j["time"] = p.sys_point; }
void from_json(const nlohmann::json& j, time_point_wrap& p) { j.at("time").get_to(p.sys_point); }

time_point_wrap::time_point_wrap() : sys_point(time_point::clock::now()) {}

time_point_wrap::time_point_wrap(
    std::int32_t in_year, std::int32_t in_month, std::int32_t in_day, std::int32_t in_hours, std::int32_t in_minutes,
    std::int32_t in_seconds
)
    : time_point_wrap(chrono::local_time_pos{
          chrono::local_days{
              chrono::year{boost::numeric_cast<std::int32_t>(in_year)} /
              chrono::month{boost::numeric_cast<std::uint32_t>(in_month)} /
              chrono::day{boost::numeric_cast<std::uint32_t>(in_day)}} +
          chrono::hours{in_hours} + chrono::minutes{in_minutes} + chrono::seconds{in_seconds}}) {}

std::int32_t time_point_wrap::get_week_int() const {
  date::weekday k_weekday{chrono::time_point_cast<date::days>(get_local_time())};
  return boost::numeric_cast<std::int32_t>(k_weekday.c_encoding());
}

time_point_wrap::compose_type time_point_wrap::compose() const {
  auto&& [l_d, l_s] = compose_1();
  date::year_month_day k_day{l_d};
  date::hh_mm_ss k_hh_mm_ss{l_s};

  return time_point_wrap::compose_type{
      boost::numeric_cast<std::uint16_t>((std::int32_t)k_day.year()),
      boost::numeric_cast<std::uint16_t>((std::uint32_t)k_day.month()),
      boost::numeric_cast<std::uint16_t>((std::uint32_t)k_day.day()),
      boost::numeric_cast<std::uint16_t>(k_hh_mm_ss.hours().count()),
      boost::numeric_cast<std::uint16_t>(k_hh_mm_ss.minutes().count()),
      boost::numeric_cast<std::uint16_t>(k_hh_mm_ss.seconds().count())};
}
time_point_wrap::compose_2_type time_point_wrap::compose_1() const {
  auto k_local = get_local_time();
  auto k_dp    = chrono::floor<chrono::days>(k_local);
  return time_point_wrap::compose_2_type{k_dp, chrono::floor<std::chrono::seconds>(k_local - k_dp)};
}
bool time_point_wrap::operator==(const time_point_wrap& in_rhs) const { return sys_point == in_rhs.sys_point; }
bool time_point_wrap::operator<(const time_point_wrap& in_rhs) const { return sys_point < in_rhs.sys_point; }

time_point_wrap time_point_wrap::current_month_end(const time_point_wrap& in_time) {
  auto&& [l_y, l_m, l_d, l_1, l_2, l_3] = in_time.compose();
  auto l_mo                             = chrono::year{l_y} / chrono::month{l_m} / chrono::last;
  return time_point_wrap{chrono::local_days{l_mo} + 23h + 59min + 59s};
}
time_point_wrap time_point_wrap::min() { return time_point_wrap{time_point::min()}; }
time_point_wrap time_point_wrap::max() { return time_point_wrap{time_point::max()}; }
time_point_wrap time_point_wrap::current_month_start(const time_point_wrap& in_time) {
  auto&& [l_y, l_m, l_d, l_1, l_2, l_3] = in_time.compose();
  auto l_mo                             = chrono::year{l_y} / chrono::month{l_m - 1u} / chrono::last;
  return time_point_wrap{chrono::local_days{l_mo} + doodle::chrono::days{1}};
}
time_point_wrap time_point_wrap::current_month_end() const { return current_month_end(*this); }
time_point_wrap time_point_wrap::current_month_start() const { return current_month_start(*this); }

void time_point_wrap::set_time(const time_point_wrap::time_local_point& in) { sys_point = to_sys_point(in); }
void time_point_wrap::set_time(const time_point_wrap::time_point& in) { sys_point = in; }

time_point_wrap time_point_wrap::now() { return time_point_wrap{time_point::clock::now()}; }
time_point_wrap::time_point time_point_wrap::get_sys_time() const { return sys_point; }
time_point_wrap::time_local_point time_point_wrap::get_local_time() const { return to_local_point(sys_point); }

time_point_wrap::~time_point_wrap() = default;

time_point_wrap& time_point_wrap::operator+=(const time_point_wrap::duration& in_dur) {
  sys_point = get_sys_time() + in_dur;
  return *this;
}
time_point_wrap& time_point_wrap::operator-=(const time_point_wrap::duration& in_dur) {
  sys_point = get_sys_time() - in_dur;
  return *this;
}
time_point_wrap& time_point_wrap::operator++() {
  sys_point = time_point{++get_sys_time().time_since_epoch()};
  return *this;
}
time_point_wrap& time_point_wrap::operator--() {
  sys_point = time_point{--get_sys_time().time_since_epoch()};
  return *this;
}
time_point_wrap::operator std::tm() const {
  auto l_time_t = chrono::to_time_t(get_local_time());
  std::tm l_tm{};
  auto l_i = localtime_s(&l_tm, &l_time_t);
  if (!l_i) throw_error(error_enum::time_to_tm_error, "时间转换出错");
  return l_tm;
}
time_point_wrap::time_point time_point_wrap::get_local_point_to_fmt_lib() const {
  return chrono::clock_cast<time_point::clock>(sys_point);
}
time_point_wrap::time_point time_point_wrap::get_sys_point_to_fmt_lib() const {
  return to_sys_point(chrono::clock_cast<time_local_point::clock>(sys_point));
}

time_point_wrap::duration operator-(const time_point_wrap& in_l, const time_point_wrap& in_r) {
  return in_l.get_sys_time() - in_r.get_sys_time();
}
}  // namespace doodle
