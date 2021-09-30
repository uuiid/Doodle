//
// Created by TD on 2021/9/27.
//

#pragma once
#include <DoodleLib/Gui/base_windwos.h>
#include <DoodleLib/doodleLib_fwd.h>
namespace doodle {
class DOODLELIB_API time_widget : public base_widget {
  time_wrap_ptr p_time;

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

  void set_time(const time_wrap_ptr& in_time);
};
}  // namespace doodle
