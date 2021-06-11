//
// Created by TD on 2021/6/11.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API TimeWidget : public wxFrame{
  std::chrono::time_point<std::chrono::system_clock> p_time;

  wxSpinCtrl* p_year_ctrl;
  wxSpinCtrl* p_month_ctrl;
  wxSpinCtrl* p_day_ctrl;
  wxSpinCtrl* p_hour_ctrl;
  wxSpinCtrl* p_minute_ctrl;
  wxSpinCtrl* p_second_ctrl;

  wxStaticText* p_week_ctrl;
  wxStaticText* p_str_ctrl;

  void init();
 public:
  explicit TimeWidget(wxWindow* in_parent,std::chrono::time_point<std::chrono::system_clock>& in_timePoint);

  static std::chrono::time_point<std::chrono::system_clock> get_time();
};

}  // namespace doodle
