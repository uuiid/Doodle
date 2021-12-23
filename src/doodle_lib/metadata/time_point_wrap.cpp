//
// Created by TD on 2021/5/17.
//

#include <Metadata/metadata.h>
#include <date/date.h>
#include <date/tz.h>
#include <doodle_lib/metadata/time_point_wrap.h>

namespace doodle {

time_point_wrap::time_point_wrap()
    : p_time(),
      p_year(),
      p_month(),
      p_day(),
      p_hours(),
      p_minutes(),
      p_seconds(),
      p_time_zone(date::current_zone()) {
  disassemble(std::chrono::system_clock::now());
}
time_point_wrap::time_point_wrap(time_point in_utc_timePoint)
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

std::uint16_t time_point_wrap::get_year() const {
  return (int)p_year;
}
void time_point_wrap::set_year(std::uint16_t in_year) {
  p_year = date::year{in_year};
  disassemble();
}
std::uint16_t time_point_wrap::get_month() const {
  return (std::uint32_t)p_month;
}
void time_point_wrap::set_month(std::uint16_t in_month) {
  p_month = date::month{in_month};
  disassemble();
}
std::uint16_t time_point_wrap::get_day() const {
  return (std::uint32_t)p_day;
}
void time_point_wrap::set_day(std::uint16_t in_day) {
  p_day = date::day{in_day};
  disassemble();
}
std::uint16_t time_point_wrap::get_hour() const {
  return p_hours.count();
}
void time_point_wrap::set_hour(std::uint16_t in_hour) {
  p_hours = std::chrono::hours{in_hour};
  disassemble();
}
std::uint16_t time_point_wrap::get_minutes() const {
  return p_minutes.count();
}
void time_point_wrap::set_minutes(std::uint16_t in_minutes) {
  p_minutes = std::chrono::minutes{in_minutes};
  disassemble();
}
std::uint16_t time_point_wrap::get_second() const {
  return p_seconds.count();
}
void time_point_wrap::set_second(std::uint16_t in_second) {
  p_seconds = std::chrono::seconds{in_second};
  disassemble();
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
  date::weekday k_weekday{date::local_days{p_year / p_month / p_day}};
  return k_weekday.c_encoding();
}

std::string time_point_wrap::show_str() const {
  return date::format("%Y/%m/%d %H:%M:%S", get_local_time());
}
time_point_wrap::time_point time_point_wrap::get_utc_time() const {
  return p_time;
}

void time_point_wrap::disassemble() {
  auto k_time = date::local_days{p_year / p_month / p_day} + p_hours + p_minutes + p_seconds;
  auto k_     = date::make_zoned(p_time_zone, k_time);
  disassemble(k_.get_sys_time());
}
chrono::local_time<chrono::seconds> time_point_wrap::get_local_time() const {
  auto k_time = date::local_days{p_year / p_month / p_day} + p_hours + p_minutes + p_seconds;
  // auto k_time2 = k_time - k_time;
  // time_point test = date::clock_cast<std::chrono::system_clock>(k_time);
  return k_time;
}

chrono::hours_double time_point_wrap::work_duration(const time_point_wrap& in) const {
  /// @warning 开始时间不能比结束时间大
  if (p_time > in.p_time)
    return chrono::hours_double{0};

  auto k_begin                  = date::sys_days{p_year / p_month / p_day};
  auto k_end                    = date::sys_days{in.p_year / in.p_month / in.p_day};

  auto k_time                   = k_end - k_begin;  /// 总总工作天数()
  /// 这里要测试工作和休息日
  chrono::hours_double k_time_h = work_days(k_begin, k_end).count() * chrono::hours_double{8};  /// 总工作小时

  /**
   *  @warning 首先是加入开始， 并且加入结束
   *  所以减去开始时多出来的部分， 再减去结束时多出来的部分
   *  k_time_h = (k_time.count() * chrono::hours_double{8})
   *  - one_day_works_hours(p_time)
   *  + one_day_works_hours(in.p_time);
   *  简化为
   *  k_time_h = k_time_h + one_day_works_hours(p_time) - one_day_works_hours(in.p_time);
   */
  k_time_h                      = k_time_h -
             (chrono::is_rest_day(k_begin)
                  ? chrono::hours_double{0}
                  : one_day_works_hours(get_local_time())) +
             (chrono::is_rest_day(k_end)
                  ? chrono::hours_double{0}
                  : one_day_works_hours(in.get_local_time()));

  return k_time_h;
}

void time_point_wrap::disassemble(const chrono::sys_time_pos& in_utc_timePoint) {
  // date::locate_zone("");
  // date::to_utc_time(p_time);
  p_time       = in_utc_timePoint;
  auto k_local = date::make_zoned(p_time_zone, in_utc_timePoint).get_local_time();

  auto k_dp    = date::floor<date::days>(k_local);
  date::year_month_day k_day{k_dp};
  date::hh_mm_ss k_hh_mm_ss{date::floor<std::chrono::milliseconds>(k_local - k_dp)};
  p_year    = k_day.year();
  p_month   = k_day.month();
  p_day     = k_day.day();
  p_hours   = k_hh_mm_ss.hours();
  p_minutes = k_hh_mm_ss.minutes();
  p_seconds = k_hh_mm_ss.seconds();
}
time_point_wrap::operator time_point() {
  return p_time;
}

chrono::hours_double time_point_wrap::one_day_works_hours(const chrono::local_time<chrono::seconds>& in_point) const {
  /// 获得当天的日期后制作工作时间
  auto k_day     = chrono::floor<chrono::days>(in_point);

  auto k_begin_1 = k_day + std::chrono::hours{9};   ///上午上班时间
  auto k_end_1   = k_day + std::chrono::hours{12};  /// 上午下班时间
  auto k_begin_2 = k_day + std::chrono::hours{13};  /// 下午上班时间
  auto k_end_2   = k_day + std::chrono::hours{18};  /// 下午下班时间

  chrono::hours_double k_h{0};
  if (in_point <= k_begin_1) {                               ///上班前提交
                                                             ///
  } else if (in_point > k_begin_1 && in_point <= k_end_1) {  ///上午上班后提交
    k_h = in_point - k_begin_1;                              ///
                                                             ///
  } else if (in_point > k_end_1 && in_point <= k_begin_2) {  /// 中文午休提交
    k_h = chrono::hours_double{3};                           ///
                                                             ///
  } else if (in_point > k_begin_2 && in_point <= k_end_2) {  /// 下午上班后提交
    k_h = chrono::hours_double{3} + (in_point - k_begin_2);  ///
                                                             ///
  } else if (in_point > k_end_2) {                           /// 下午下班后提交
    k_h = chrono::hours_double{8};                           ///
  } else {
    chick_true<doodle_error>(false, DOODLE_LOC, "未知时间");
  }
  return k_h;
}
chrono::days time_point_wrap::work_days(const time_point_wrap::time_point& in_begin,
                                        const time_point_wrap::time_point& in_end) const {
  auto k_day_begin = chrono::floor<chrono::days>(in_begin);
  auto k_day_end   = chrono::floor<chrono::days>(in_end);

  std::vector<chrono::sys_days> k_days{};
  k_days.resize((k_day_end - k_day_begin).count());
  std::generate_n(k_days.begin(), (k_day_end - k_day_begin).count(), [k_day_begin, n = 0]() mutable -> chrono::sys_days {
    auto k_i = k_day_begin + chrono::days{n};
    ++n;
    return k_i;
  });
  auto k_s = std::count_if(k_days.begin(), k_days.end(), [](const chrono::sys_days& in) {
    return !chrono::is_rest_day(in);
  });
  return chrono::days{k_s};
}
void time_point_wrap::set_local_time(const date::local_time<chrono::seconds>& in_time) {
  auto k_time = chrono::make_zoned(p_time_zone, in_time);
  disassemble(k_time.get_sys_time());
}

void time_point_wrap::set_time(const chrono::sys_time_pos& in_time) {
  disassemble(in_time);
}

void time_point_wrap::set_time(const chrono::local_time_pos& in_time) {
  auto k_time = chrono::make_zoned(p_time_zone, in_time);
  disassemble(k_time.get_sys_time());
}

std::time_t time_point_wrap::get_local_time_t() const {
  auto k_time = get_local_time();
  return chrono::to_time_t(k_time);
}
std::time_t time_point_wrap::get_utc_time_t() const {
  return time_point::clock::to_time_t(p_time);
}

}  // namespace doodle
