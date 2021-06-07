//
// Created by TD on 2021/5/17.
//

#include <DoodleLib/Metadata/TimeDuration.h>
#include <date/date.h>
namespace doodle {

TimeDuration::TimeDuration()
    : p_start_point(std::chrono::system_clock::now()),
      p_end_point(std::chrono::system_clock::now()),
      p_duration() {
}
TimeDuration::TimeDuration(time_point in_point)
    : p_start_point(in_point),
      p_end_point(in_point),
      p_duration() {
}

TimeDuration::time_point TimeDuration::startPoint() const {
  return p_start_point;
}
void TimeDuration::setStartPoint(const time_point& in_point) {
  p_start_point = in_point;
}

TimeDuration::time_point TimeDuration::endPoint() const {
  return p_end_point;
}
void TimeDuration::setEndPoint(const time_point& in_point) {
  p_end_point     = in_point;
  auto k_duration = p_end_point - p_start_point;
  p_duration      = std::chrono::duration_cast<std::chrono::minutes>(k_duration);
}

std::chrono::minutes TimeDuration::duration() const {
  return p_duration;
}
std::chrono::minutes TimeDuration::weekdaysDuration() const {
  return p_duration;
}

}  // namespace doodle
