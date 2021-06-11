//
// Created by TD on 2021/5/17.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>
#include <date/date.h>
namespace doodle{



class TimeDuration {
  date::year p_year;
  date::month p_month;
  date::day p_day;
  std::chrono::hours p_hours;
  std::chrono::minutes p_minutes;
  std::chrono::seconds p_seconds;
  

 public:
  using time_point = std::chrono::time_point<std::chrono::system_clock>;
  TimeDuration();
  explicit TimeDuration(time_point in_point);

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
  [[nodiscard]] time_point getTime() const;

 private:
  //这里是序列化的代码
  friend class cereal::access;
  template <class Archive>
  void serialize(Archive &ar, std::uint32_t const version);
};
template <class Archive>
void TimeDuration::serialize(Archive& ar, const std::uint32_t version) {
  if(version == 1)
    ar(
        cereal::make_nvp("year",p_year),
        cereal::make_nvp("month",p_month),
        cereal::make_nvp("days",p_day),
        cereal::make_nvp("hours",p_hours),
        cereal::make_nvp("minutes",p_minutes),
        cereal::make_nvp("seconds",p_seconds)
        );
}

}
CEREAL_CLASS_VERSION(doodle::TimeDuration,1)
