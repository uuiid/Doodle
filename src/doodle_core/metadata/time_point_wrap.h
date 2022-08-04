//
// Created by TD on 2021/5/17.
//

#pragma once
#include <date/date.h>
#include <date/tz.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/operators.hpp>
namespace doodle {
class time_point_wrap;
void to_json(nlohmann::json& j, const time_point_wrap& p);
void from_json(const nlohmann::json& j, time_point_wrap& p);

/**
 * @brief 这是一个小的时间类
 * @warning 这个类中的设置时间的函数和都是设置本地日期的，并不是utc时间， 他会自动在内部转换为utc
 */
class DOODLE_CORE_EXPORT time_point_wrap
    : boost::totally_ordered<time_point_wrap> {
 public:
  using time_point       = chrono::sys_time_pos;
  using time_duration    = time_point::duration;
  using time_local_point = chrono::local_time<time_duration>;
  using time_zoned       = chrono::zoned_time<time_duration>;

 private:
  void set_time(const time_local_point& in);
  void set_time(const time_point& in);
  void set_time(const time_zoned& in);

 public:
  chrono::zoned_time<time_duration> zoned_time_;
  time_point_wrap();
  explicit time_point_wrap(const time_zoned& in_time_zoned);
  explicit time_point_wrap(time_point in_utc_timePoint);
  explicit time_point_wrap(time_local_point in_utc_timePoint);
  explicit time_point_wrap(
      std::int32_t in_year,
      std::int32_t in_month,
      std::int32_t in_day,
      std::int32_t in_hours   = 0,
      std::int32_t in_minutes = 0,
      std::int32_t in_seconds = 0);

  [[nodiscard]] std::tuple<std::uint16_t,  // year
                           std::uint16_t,  // month
                           std::uint16_t,  // day
                           std::uint16_t,  // hours
                           std::uint16_t,  // minutes
                           std::uint16_t>  // seconds
  compose() const;

  [[nodiscard]] std::string get_week_s() const;
  [[nodiscard]] std::int32_t get_week_int() const;

  [[nodiscard]] std::string show_str() const;

  static time_point_wrap current_month_end(const time_point_wrap& in_time);
  static time_point_wrap current_month_start(const time_point_wrap& in_time);

  [[nodiscard]] time_point_wrap current_month_end() const;
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

  bool operator==(const time_point_wrap& in_rhs) const;
  bool operator<(const time_point_wrap& in_rhs) const;

  template <typename Rep_T, typename Period_T>
  time_point_wrap& operator+=(const doodle::chrono::duration<Rep_T, Period_T>& in_dur) {
    auto l_sys_time = zoned_time_.get_sys_time();
    l_sys_time += in_dur;
    zoned_time_ = doodle::chrono::make_zoned(zoned_time_.get_time_zone(), l_sys_time);
    return *this;
  }
  template <typename Rep_T, typename Period_T>
  time_point_wrap& operator-=(const doodle::chrono::duration<Rep_T, Period_T>& in_dur) {
    auto l_sys_time = zoned_time_.get_sys_time();
    l_sys_time -= in_dur;
    zoned_time_ = doodle::chrono::make_zoned(zoned_time_.get_time_zone(), l_sys_time);
    return *this;
  }
  template <typename Rep_T, typename Period_T>
  time_point_wrap operator+(const doodle::chrono::duration<Rep_T, Period_T>& in_dur) {
    auto l_sys_time = zoned_time_.get_sys_time();
    l_sys_time += in_dur;
    return time_point_wrap{doodle::chrono::make_zoned(zoned_time_.get_time_zone(), l_sys_time)};
  }

  template <typename Rep_T, typename Period_T>
  time_point_wrap operator-(const doodle::chrono::duration<Rep_T, Period_T>& in_dur) {
    auto l_sys_time = zoned_time_.get_sys_time();
    l_sys_time -= in_dur;
    return time_point_wrap{doodle::chrono::make_zoned(zoned_time_.get_time_zone(), l_sys_time)};
  }

  template <class Clock,
            class Duration = typename Clock::duration>
  time_point_wrap& operator=(const doodle::chrono::time_point<Clock, Duration>& in_dur) {
    this->set_time(chrono::floor<time_duration>(in_dur));
    return *this;
  }

 private:
  // 这里是序列化的代码
  friend void to_json(nlohmann::json& j, const time_point_wrap& p);
  friend void from_json(const nlohmann::json& j, time_point_wrap& p);
};

}  // namespace doodle

namespace fmt {
/**
 * @brief 格式化时间包裹类
 *
 * @tparam 资产类
 */
template <>
struct formatter<::doodle::time_point_wrap> : formatter<std::string_view> {
  /**
   * @brief 格式化函数
   *
   * @tparam FormatContext fmt上下文类
   * @param in_ 传入的资产类
   * @param ctx 上下文
   * @return decltype(ctx.out()) 基本上时 std::string
   */
  template <typename FormatContext>
  auto format(const ::doodle::time_point_wrap& in_, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<std::string_view>::format(
        in_.show_str(),
        ctx);
  }
};
}  // namespace fmt
