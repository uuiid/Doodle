//
// Created by TD on 2021/5/17.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <DoodleLib/Exception/exception.h>
#include <date/date.h>
#include <date/tz.h>

namespace doodle {

/**
 * @brief 这是一个小的时间类
 * @warning 这个类中的设置时间的函数和都是设置本地日期的，并不是utc时间， 他会自动在内部转换为utc
 */
class DOODLELIB_API time_point_wrap : public details::no_copy {
  /**
   * @brief 这个是内部的utc时间
   *
   */
  chrono::sys_time_pos p_time;
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
  using time_point = chrono::sys_time_pos;
  time_point_wrap();
  explicit time_point_wrap(time_point in_utc_timePoint);

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

  [[nodiscard]] std::string get_week_s() const;

  [[nodiscard]] std::int32_t get_week_int() const;

  [[nodiscard]] std::string show_str() const;
  [[nodiscard]] time_point get_utc_time() const;
  [[nodiscard]] chrono::local_time<chrono::seconds> get_local_time() const;
  [[nodiscard]] std::time_t get_local_time_t() const;
  [[nodiscard]] std::time_t get_utc_time_t() const;
  void set_local_time(const chrono::local_time<chrono::seconds>& in_time);

  /**
   *
   * @param in 结束的时间
   * @return
   *
   * @todo 时间选项中去除节假日和个人调休
   *
   */
  [[nodiscard]] chrono::hours_double work_duration(const time_point_wrap& in) const;

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
   *
   * @todo: 这里我们要添加设置， 而不是静态变量
   */
  chrono::hours_double one_day_works_hours(const chrono::local_time<chrono::seconds>& in_point) const;
  chrono::days work_days(const time_point& in_begin, const time_point& in_end) const;

  void disassemble();

  void disassemble(const chrono::sys_time_pos& in_utc_timePoint);

  //这里是序列化的代码
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const std::uint32_t version) {
    if (version == 1)
      ar& boost::serialization::make_nvp("time", p_time);
    disassemble(p_time);
  };
};

// class time_point : public std::chrono::time_point<std::chrono::system_clock> {
//  public:
//   time_point(){
//   };
// }
}  // namespace doodle
BOOST_CLASS_VERSION(doodle::time_point_wrap, 1)
BOOST_CLASS_EXPORT_KEY(doodle::time_point_wrap)
