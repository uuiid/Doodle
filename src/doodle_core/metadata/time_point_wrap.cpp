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
  auto l_time_t = chrono::to_time_t(p_i->zoned_time_.get_local_time());
  std::tm l_tm{};
  auto l_i = localtime_s(&l_tm, &l_time_t);
  if (!l_i)
    throw_error(error_enum::time_to_tm_error, "时间转换出错");
  return l_tm;
}
time_point_wrap::time_point time_point_wrap::get_local_point_to_fmt_lib() const {
  return chrono::clock_cast<chrono::system_clock>(p_i->zoned_time_.get_local_time());
}
time_point_wrap::time_point time_point_wrap::get_sys_point_to_fmt_lib() const {
  return p_i->zoned_time_.get_sys_time();
}

time_point_wrap::duration operator-(const time_point_wrap& in_l, const time_point_wrap& in_r) {
  return in_l.get_sys_time() - in_r.get_sys_time();
}
}  // namespace doodle
