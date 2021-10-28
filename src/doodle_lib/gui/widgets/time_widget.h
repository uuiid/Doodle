//
// Created by TD on 2021/9/27.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/gui/base_windwos.h>

#include <boost/signals2.hpp>
namespace doodle {
class DOODLELIB_API time_widget : public base_widget {
  entt::handle p_time;

  std::int32_t p_year;
  std::int32_t p_month;
  std::int32_t p_day;
  std::int32_t p_hour;
  std::int32_t p_minutes;
  std::int32_t p_second;

  std::int32_t p_day_max;
 public:
  time_widget();
  void frame_render() override;

  void set_time(const entt::handle& in_time);
  boost::signals2::signal<void()> sig_time_change;
};
}  // namespace doodle
