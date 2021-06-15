//
// Created by TD on 2021/6/11.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle {
class DOODLELIB_API TimeWidget : public wxDialog{
  TimeDurationPtr p_time;

  wxSpinCtrl* p_year_ctrl;
  wxSpinCtrl* p_month_ctrl;
  wxSpinCtrl* p_day_ctrl;
  wxSpinCtrl* p_hour_ctrl;
  wxSpinCtrl* p_minute_ctrl;
  wxSpinCtrl* p_second_ctrl;

  wxStaticText* p_week_ctrl;
  wxStaticText* p_str_ctrl;

  wxButton* p_button_ok;
  wxButton* p_button_cancel;


  void init();
  void bind_fun();
  inline void set_week_text();
  inline void set_str_text();

 public:
  explicit TimeWidget(wxWindow* in_parent,TimeDurationPtr& in_timePoint);

  static  TimeDurationPtr get_time(wxWindow* in_parent);
};

}  // namespace doodle
