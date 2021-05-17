//
// Created by TD on 2021/5/17.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle{
class TimeDuration {
  std::chrono::time_point<std::chrono::system_clock> p_start_point;
  std::chrono::time_point<std::chrono::system_clock> p_end_point;
  std::chrono::minutes p_duration;

 public:
  using time_point = std::chrono::time_point<std::chrono::system_clock>;
  TimeDuration();
  explicit TimeDuration(time_point in_point);

  [[nodiscard]] time_point startPoint() const;
  void setStartPoint(const time_point& in_point = std::chrono::system_clock::now());
  [[nodiscard]] time_point endPoint() const;
  void setEndPoint(const time_point& in_point = std::chrono::system_clock::now());

  [[nodiscard]] std::chrono::minutes duration() const;
  [[nodiscard]] std::chrono::minutes weekdaysDuration() const;

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
        cereal::make_nvp("start_point",p_start_point),
        cereal::make_nvp("end_point",p_end_point),
        cereal::make_nvp("duration",p_duration)
        );
}
}
CEREAL_CLASS_VERSION(doodle::TimeDuration,1)
