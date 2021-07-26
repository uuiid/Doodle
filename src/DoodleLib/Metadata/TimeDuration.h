//
// Created by TD on 2021/5/17.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/Exception.h>
#include <date/date.h>
#include <date/tz.h>

#include <cereal/types/chrono.hpp>

namespace doodle {

/**
 * @brief 这是一个小的时间类
 * @warning 这个类中的设置时间的函数和都是设置本地日期的，并不是utc时间， 他会自动在内部转换为utc
 */
class DOODLELIB_API TimeDuration : public details::no_copy {
  /**
   * @brief 这个是内部的utc时间
   * 
   */
  std::chrono::time_point<std::chrono::system_clock> p_time;
  /**
   * @brief 这里是本地日期的年组件之一
   * 
   */
  date::year p_year;
  date::month p_month;
  date::day p_day;

  std::chrono::hours p_hours;
  std::chrono::minutes p_minutes;
  std::chrono::seconds p_seconds;

  /**
   * @brief 这个是指向时区的指针
   *  每次创建时都会重新获取当前的时区，并和系统时间组合为本地时间 
   */
  const date::time_zone* p_time_zone;

 public:
  using time_point = std::chrono::time_point<std::chrono::system_clock>;
  TimeDuration();
  explicit TimeDuration(time_point in_utc_timePoint);

  [[nodiscard]] std::uint16_t get_year() const;
  void set_year(std::uint16_t in_year);

  [[nodiscard]] std::uint16_t get_month() const;
  void set_month(std::uint16_t in_month);

  [[nodiscard]] std::uint16_t get_day() const;
  void set_day(std::uint16_t in_day);

  [[nodiscard]] std::uint16_t get_hour() const;
  void set_hour(std::uint16_t in_hour);

  [[nodiscard]] std::uint16_t get_minutes() const;
  void set_minutes(std::uint16_t in_minutes);

  [[nodiscard]] std::uint16_t get_second() const;
  void set_second(std::uint16_t in_second);

  template <class T>
  [[nodiscard]] T getWeek() const {};

  template <>
  [[nodiscard]] std::string getWeek() const;

  template <>
  [[nodiscard]] std::int32_t getWeek() const;

  [[nodiscard]] std::string showStr() const;
  [[nodiscard]] time_point getUTCTime() const;
  [[nodiscard]] time_point getLocalTime() const;
  void set_local_time(const chrono::local_time<chrono::seconds>& in_time);

  //  template <class T, class P>
  //  inline TimeDuration& operator-=(const std::chrono::duration<T, P>& _or) {
  //    p_time -= _or;
  //    disassemble(p_time);
  //    return *this;
  //  }
  /**
   *
   * @param in 结束的时间
   * @return
   */
  chrono::hours_double work_duration(const TimeDuration& in) const;

  /**
   * 这里返回系统时钟 系统时钟我们始终假定为 utc时钟 (system_clock跟踪的时区（未指定但事实上的标准）)
   * @warning 这里一定要注意, 在库中还存在一种用来表示本地时间的系统时钟,
   * 将 system_clock 用于表示本地时间的原因主要是为了和其他库进行快速的接口使用
   *
   * TODO: 这里我们因该将上面的表示为系统时钟的本地时间进行重构
   * @return
   */
  operator time_point();

 private:
  /**
   * 这个是计算开始时到一天结束时的工作时长
   * 通常是安装8小时计算, 同时计算前段时间可以使用 8-return
   * @param in_point  开始的时间点
   * @return 工作时间长度
   */
  chrono::hours_double one_day_works_hours(const time_point& in_point) const;
  chrono::days work_days(const time_point& in_begin, const time_point& in_end) const;

  void disassemble();

  void disassemble(const std::chrono::time_point<std::chrono::system_clock>& in_utc_timePoint);

  //这里是序列化的代码
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive& ar, const std::uint32_t version);
};
template <class Archive>
void TimeDuration::serialize(Archive& ar, const std::uint32_t version) {
  if (version == 1)
    ar(
        cereal::make_nvp("year", p_year),
        cereal::make_nvp("month", p_month),
        cereal::make_nvp("days", p_day),
        cereal::make_nvp("hours", p_hours),
        cereal::make_nvp("minutes", p_minutes),
        cereal::make_nvp("seconds", p_seconds),
        cereal::make_nvp("time", p_time));
  disassemble(p_time);
}

// class time_point : public std::chrono::time_point<std::chrono::system_clock> {
//  public:
//   time_point(){
//   };
// }
}  // namespace doodle
CEREAL_CLASS_VERSION(doodle::TimeDuration, 1)
