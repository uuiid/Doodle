 //
// Created by TD on 2021/5/17.
//

#pragma once
#include <date/date.h>
#include <date/tz.h>
#include <doodle_core/doodle_core_fwd.h>

namespace doodle {

/**
 * @brief 这是一个小的时间类
 * @warning 这个类中的设置时间的函数和都是设置本地日期的，并不是utc时间， 他会自动在内部转换为utc
 */
class DOODLE_CORE_EXPORT time_point_wrap {
 public:
  using time_point       = chrono::sys_time_pos;
  using time_duration    = time_point::duration;
  using time_local_point = chrono::local_time<time_duration>;
  using time_zoned       = chrono::zoned_time<time_duration>;

 private:
 public:
  class DOODLE_CORE_EXPORT gui_data {
   public:
    std::uint16_t year_;
    std::uint16_t month_;
    std::uint16_t day_;
    std::uint16_t hours_;
    std::uint16_t minutes_;
    std::uint16_t seconds_;

    gui_data() : gui_data(time_point_wrap{}) {}
    explicit gui_data(const time_point_wrap& in_wrap) {
      std::tie(year_,
               month_,
               day_,
               hours_,
               minutes_,
               seconds_) = in_wrap.compose();
    }
  };

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
  explicit time_point_wrap(const gui_data& in_data);

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
  bool operator!=(const time_point_wrap& in_rhs) const;

  bool operator<(const time_point_wrap& in_rhs) const;
  bool operator>(const time_point_wrap& in_rhs) const;
  bool operator<=(const time_point_wrap& in_rhs) const;
  bool operator>=(const time_point_wrap& in_rhs) const;

  bool operator<(const time_point& in_rhs) const;
  bool operator>(const time_point& in_rhs) const;
  bool operator<=(const time_point& in_rhs) const;
  bool operator>=(const time_point& in_rhs) const;

  bool operator<(const time_local_point& in_rhs) const;
  bool operator>(const time_local_point& in_rhs) const;
  bool operator<=(const time_local_point& in_rhs) const;
  bool operator>=(const time_local_point& in_rhs) const;

  bool operator<(const time_zoned& in_rhs) const;
  bool operator>(const time_zoned& in_rhs) const;
  bool operator<=(const time_zoned& in_rhs) const;
  bool operator>=(const time_zoned& in_rhs) const;

  template <typename Rep_T, typename Period_T>
  time_point_wrap& operator+=(const doodle::chrono::duration<Rep_T, Period_T>& in_dur){
    auto l_sys_time = zoned_time_.get_sys_time();
    l_sys_time += in_dur;
    zoned_time_ = doodle::chrono::make_zoned(zoned_time_.get_time_zone(), l_sys_time);
    return *this;
  }

  template <typename Rep_T, typename Period_T>
  time_point_wrap operator+(const doodle::chrono::duration<Rep_T, Period_T>& in_dur){
    auto l_sys_time = zoned_time_.get_sys_time();
    l_sys_time += in_dur;
    return time_point_wrap{doodle::chrono::make_zoned(zoned_time_.get_time_zone(), l_sys_time)};
  }

  template <typename Rep_T, typename Period_T>
  time_point_wrap operator-(const doodle::chrono::duration<Rep_T, Period_T>& in_dur){
    auto l_sys_time = zoned_time_.get_sys_time();
    l_sys_time -= in_dur;
    return time_point_wrap{doodle::chrono::make_zoned(zoned_time_.get_time_zone(), l_sys_time)};
  }

 private:
  /**
   * 这个是计算开始时到一天结束时的工作时长
   * 通常是安装8小时计算, 同时计算前段时间可以使用 8-return
   * @param in_point  开始的时间点
   * @return 工作时间长度
   *
   * @todo: 这里我们要添加设置， 而不是静态变量
   */
  chrono::hours_double one_day_works_hours(const time_local_point& in_point) const;
  chrono::days work_days(const time_local_point& in_begin, const time_local_point& in_end) const;

  // 这里是序列化的代码

  friend void to_json(nlohmann::json& j, const time_point_wrap& p) {
    j["time"] = p.zoned_time_.get_sys_time();
  }
  friend void from_json(const nlohmann::json& j, time_point_wrap& p) {
    p.zoned_time_ = j.at("time").get<time_point>();
  }
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
