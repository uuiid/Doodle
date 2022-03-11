//
// Created by TD on 2021/5/17.
//

#include <Metadata/metadata.h>
#include <date/date.h>
#include <date/tz.h>
#include <doodle_lib/metadata/time_point_wrap.h>

namespace doodle {

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

chrono::hours_double time_point_wrap::work_duration(const time_point_wrap& in) const {
  /// @warning 开始时间不能比结束时间大
  if (zoned_time_.get_sys_time() > in.zoned_time_.get_sys_time())
    return chrono::hours_double{0};

  auto k_begin                  = chrono::floor<chrono::days>(zoned_time_.get_local_time());
  auto k_end                    = chrono::floor<chrono::days>(in.zoned_time_.get_local_time());

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
                  : one_day_works_hours(zoned_time_.get_local_time())) +
             (chrono::is_rest_day(k_end)
                  ? chrono::hours_double{0}
                  : one_day_works_hours(in.zoned_time_.get_local_time()));

  return k_time_h;
  //  return {};
}

chrono::hours_double time_point_wrap::one_day_works_hours(const time_local_point& in_point) const {
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
chrono::days time_point_wrap::work_days(const time_point_wrap::time_local_point& in_begin,
                                        const time_point_wrap::time_local_point& in_end) const {
  auto k_day_begin = chrono::floor<chrono::days>(in_begin);
  auto k_day_end   = chrono::floor<chrono::days>(in_end);

  std::vector<chrono::local_days> k_days{};
  k_days.resize((k_day_end - k_day_begin).count());
  std::generate_n(k_days.begin(), (k_day_end - k_day_begin).count(), [k_day_begin, n = 0]() mutable -> chrono::local_days {
    auto k_i = k_day_begin + chrono::days{n};
    ++n;
    return k_i;
  });
  auto k_s = std::count_if(k_days.begin(), k_days.end(), [](const chrono::local_days& in) {
    return !chrono::is_rest_day(in);
  });
  return chrono::days{k_s};
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
time_point_wrap::time_point_wrap(const time_point_wrap::gui_data& in_data)
    : time_point_wrap(in_data.year_,
                      in_data.month_,
                      in_data.day_,
                      in_data.hours_,
                      in_data.minutes_,
                      in_data.seconds_) {
}
bool time_point_wrap::operator==(const time_point_wrap& in_rhs) const {
  return zoned_time_ == in_rhs.zoned_time_;
}
bool time_point_wrap::operator!=(const time_point_wrap& in_rhs) const {
  return !(in_rhs == *this);
}
bool time_point_wrap::operator<(const time_point_wrap& in_rhs) const {
  return zoned_time_.get_sys_time() < in_rhs.zoned_time_.get_sys_time();
}
bool time_point_wrap::operator>(const time_point_wrap& in_rhs) const {
  return in_rhs < *this;
}
bool time_point_wrap::operator<=(const time_point_wrap& in_rhs) const {
  return !(in_rhs < *this);
}
bool time_point_wrap::operator>=(const time_point_wrap& in_rhs) const {
  return !(*this < in_rhs);
}

time_point_wrap time_point_wrap::current_month_end(const time_point_wrap& in_time) {
  auto&& [l_y, l_m, l_d, l_1, l_2, l_3] = in_time.compose();
  auto l_mo                             = chrono::year{l_y} / chrono::month{l_m} / chrono::last;
  // chrono::local_days k_{l_mo};
  // time_local_point l{k_};
  return time_point_wrap{chrono::local_days{l_mo}};
}

}  // namespace doodle
