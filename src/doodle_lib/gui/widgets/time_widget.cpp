//
// Created by TD on 2021/9/27.
//

#include "time_widget.h"

#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_lib/metadata/time_point_wrap.h>
namespace doodle {
time_widget::time_widget()
    : p_time(),
      p_year(2021),
      p_month(1),
      p_day(1),
      p_hour(0),
      p_minutes(0),
      p_second(0),
      p_day_max(31) {
}

void time_widget::frame_render() {
  dear::TreeNode{"时间"} && [this]() {
    imgui::SliderInt("年", &p_year, 0, 2050);
    imgui::SliderInt("月", &p_month, 1, 12);
    imgui::SliderInt("日", &p_day, 0, p_day_max);
    imgui::SliderInt("时", &p_hour, 0, 23);
    imgui::SliderInt("分", &p_minutes, 0, 59);
    imgui::SliderInt("秒", &p_second, 0, 59);
    if (imgui::Button("设置") && p_time) {
      p_time.get<time_point_wrap>().set_year(p_year);
      p_time.get<time_point_wrap>().set_month(p_month);
      p_time.get<time_point_wrap>().set_day(p_day);
      p_time.get<time_point_wrap>().set_hour(p_hour);
      p_time.get<time_point_wrap>().set_minutes(p_minutes);
      p_time.get<time_point_wrap>().set_second(p_second);
    }
  };

  using namespace chrono::literals;
  auto k_l = chrono::year{(std::int32_t)p_year} /
             chrono::month{(std::uint32_t)p_month} /
             chrono::last;
  p_day_max = (std::uint32_t)k_l.day();
  if (p_day > p_day_max)
    p_day = p_day_max;
}

void time_widget::set_time(const entt::handle& in_time) {
  if (in_time.any_of<time_point_wrap>()) {
    p_time    = in_time;
    p_year    = p_time.get<time_point_wrap>().get_year();
    p_month   = p_time.get<time_point_wrap>().get_month();
    p_day     = p_time.get<time_point_wrap>().get_day();
    p_hour    = p_time.get<time_point_wrap>().get_hour();
    p_minutes = p_time.get<time_point_wrap>().get_minutes();
    p_second  = p_time.get<time_point_wrap>().get_second();
  }
}
}  // namespace doodle
