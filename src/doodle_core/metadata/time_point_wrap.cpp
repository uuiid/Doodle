//
// Created by TD on 2021/5/17.
//

#include <doodle_core/metadata/metadata.h>
#include <date/date.h>
#include <date/tz.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/logger/logger.h>

namespace doodle {

class time_point_wrap::impl {
 public:
  chrono::zoned_time<time_duration> zoned_time_{};
};

void to_json(nlohmann::json& j, const time_point_wrap& p) {
  j["time"] = p.p_i->zoned_time_.get_sys_time();
}
void from_json(const nlohmann::json& j, time_point_wrap& p) {
  p.p_i->zoned_time_ = j.at("time").get<time_point_wrap::time_point>();
}

time_point_wrap::time_point_wrap()
    : p_i(std::make_unique<impl>()) {
  p_i->zoned_time_ = date::make_zoned(date::current_zone(), time_point::clock::now());
}

time_point_wrap::time_point_wrap(time_point in_utc_timePoint)
    : time_point_wrap() {
  set_time(in_utc_timePoint);
}

time_point_wrap::time_point_wrap(time_local_point in_local_time_point)
    : time_point_wrap() {
  set_time(in_local_time_point);
}

time_point_wrap::time_point_wrap(
    std::int32_t in_year,
    std::int32_t in_month,
    std::int32_t in_day,
    std::int32_t in_hours,
    std::int32_t in_minutes,
    std::int32_t in_seconds
)
    : time_point_wrap(chrono::local_time_pos{
          chrono::local_days{
              chrono::year{boost::numeric_cast<std::int32_t>(in_year)} /
              chrono::month{boost::numeric_cast<std::uint32_t>(in_month)} /
              chrono::day{boost::numeric_cast<std::uint32_t>(in_day)}} +
          chrono::hours{in_hours} +
          chrono::minutes{in_minutes} +
          chrono::seconds{in_seconds}}) {
}

std::int32_t time_point_wrap::get_week_int() const {
  date::weekday k_weekday{chrono::time_point_cast<date::days>(get_local_time())};
  return boost::numeric_cast<std::int32_t>(k_weekday.c_encoding());
}

std::string time_point_wrap::show_str() const {
  return date::format("%Y/%m/%d %H:%M:%S", get_local_time());
}

time_point_wrap::compose_type
time_point_wrap::compose() const {
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
  return time_point_wrap::compose_2_type{
      k_dp,
      chrono::floor<std::chrono::seconds>(k_local - k_dp)};
}
bool time_point_wrap::operator==(const time_point_wrap& in_rhs) const {
  return p_i->zoned_time_ == in_rhs.p_i->zoned_time_;
}
bool time_point_wrap::operator<(const time_point_wrap& in_rhs) const {
  return p_i->zoned_time_.get_sys_time() < in_rhs.p_i->zoned_time_.get_sys_time();
}

time_point_wrap time_point_wrap::current_month_end(const time_point_wrap& in_time) {
  auto&& [l_y, l_m, l_d, l_1, l_2, l_3] = in_time.compose();
  auto l_mo                             = chrono::year{l_y} / chrono::month{l_m} / chrono::last;
  return time_point_wrap{chrono::local_days{l_mo} + 23h + 59min + 59s};
}
time_point_wrap time_point_wrap::min() {
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
  p_i->zoned_time_ = in;
}
void time_point_wrap::set_time(const time_point_wrap::time_point& in) {
  p_i->zoned_time_ = in;
}

time_point_wrap time_point_wrap::now() {
  return time_point_wrap{time_point::clock::now()};
}
time_point_wrap::time_point time_point_wrap::get_sys_time() const {
  return p_i->zoned_time_.get_sys_time();
}
time_point_wrap::time_local_point time_point_wrap::get_local_time() const {
  return p_i->zoned_time_.get_local_time();
}

time_point_wrap::~time_point_wrap() = default;
time_point_wrap::time_point_wrap(const time_point_wrap& in_other) noexcept
    : p_i(std::make_unique<impl>()) {
  *p_i = *in_other.p_i;
}
time_point_wrap::time_point_wrap(time_point_wrap&& in_other) noexcept
    : p_i(std::move(in_other.p_i)) {
}
time_point_wrap& time_point_wrap::operator=(const time_point_wrap& in_other) noexcept {
  *p_i = *in_other.p_i;
  return *this;
}
time_point_wrap& time_point_wrap::operator=(time_point_wrap&& in_other) noexcept {
  p_i = std::move(in_other.p_i);
  return *this;
}
time_point_wrap& time_point_wrap::operator+=(const time_point_wrap::duration& in_dur) {
  p_i->zoned_time_ = get_sys_time() + in_dur;
  return *this;
}
time_point_wrap& time_point_wrap::operator-=(const time_point_wrap::duration& in_dur) {
  p_i->zoned_time_ = get_sys_time() - in_dur;
  return *this;
}
time_point_wrap& time_point_wrap::operator++() {
  p_i->zoned_time_ = time_point{++get_sys_time().time_since_epoch()};
  return *this;
}
time_point_wrap& time_point_wrap::operator--() {
  p_i->zoned_time_ = time_point{--get_sys_time().time_since_epoch()};
  return *this;
}
time_point_wrap::operator std::tm() const {
  std::tm l_tm{};
  auto l_com    = compose();
  l_tm.tm_year  = l_com.year - 1900;  /// 年份减去 1900
  l_tm.tm_mon   = l_com.month;
  l_tm.tm_mday  = l_com.day;
  /// 时分秒
  l_tm.tm_hour  = l_com.hours;
  l_tm.tm_min   = l_com.minutes;
  l_tm.tm_sec   = l_com.seconds;
  /// 星期
  l_tm.tm_wday  = boost::numeric_cast<std::int32_t>(get_week_int());

  auto l_com2   = compose_1();

  /// 当年的天数
  auto l_yday   = l_com2.y_m_d - chrono::local_days{chrono::year{l_com.year} / chrono::month{} / chrono::day{}};
  l_tm.tm_yday  = l_yday.count();
  l_tm.tm_isdst = -1;  /// 夏令时标志。如果 DST 生效，则值为正，如果没有，则为零，如果没有可用信息，则值为负
  return l_tm;
}

time_point_wrap::duration operator-(const time_point_wrap& in_l, const time_point_wrap& in_r) {
  return in_l.get_sys_time() - in_r.get_sys_time();
}
}  // namespace doodle
