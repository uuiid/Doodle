//
// Created by TD on 2021/6/11.
//

#include "TimeWidget.h"

#include <core/Util.h>
#include <wx/spinctrl.h>
#include <date/date.h>

namespace doodle {

TimeWidget::TimeWidget(wxWindow* in_parent, std::chrono::time_point<std::chrono::system_clock>& in_timePoint)
    : wxFrame(in_parent, wxID_ANY, ConvStr<wxString>("获得时间")),
      p_time(in_timePoint),
      p_year_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_month_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_day_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_hour_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_minute_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_second_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_week_ctrl(new wxStaticText{this, NewControlId(),wxEmptyString}),
      p_str_ctrl(new wxStaticText{this, NewControlId(),wxEmptyString})
{
  p_year_ctrl->SetRange(2000,3000);
  p_month_ctrl->SetRange(1,12);
  p_day_ctrl->SetRange(1,31);
  p_hour_ctrl->SetRange(0,23);
  p_minute_ctrl->SetRange(0,59);
  p_second_ctrl->SetRange(0,59);

  auto layout = wxBoxSizer(wxOrientation::wxVERTICAL);
  auto k_l_ =  wxUtil::labelAndWidget(this,"年: ",p_year_ctrl);
  layout.Add(k_l_,wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this,"月: ",p_month_ctrl);
  layout.Add(k_l_,wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this,"日: ",p_day_ctrl);
  layout.Add(k_l_,wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this,"星期： ",p_week_ctrl);
  layout.Add(k_l_,wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this,"小时: ",p_hour_ctrl);
  layout.Add(k_l_,wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this,"分钟: ",p_minute_ctrl);
  layout.Add(k_l_,wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this,"秒: ",p_second_ctrl);
  layout.Add(k_l_,wxSizerFlags{1}.Expand());
  layout.Add(wxUtil::labelAndWidget(this,"预览: ",p_str_ctrl),wxSizerFlags{1}.Expand())



}
std::chrono::time_point<std::chrono::system_clock> TimeWidget::get_time() {
  return std::chrono::time_point<std::chrono::system_clock>();
}
void TimeWidget::init() {
  auto k_dp = date::floor<date::days>(p_time);
  date::year_month_day k_day{k_dp};
  date::hh_mm_ss k_hh_mm_ss{date::floor<std::chrono::milliseconds>(p_time - k_dp)};
  p_year_ctrl->SetValue((int)k_day.year());
  p_month_ctrl->SetValue((unsigned int)k_day.month());
  p_day_ctrl->SetValue((unsigned int)k_day.day());
  p_hour_ctrl->SetValue(k_hh_mm_ss.hours().count());
  p_minute_ctrl->SetValue(k_hh_mm_ss.minutes().count());
  p_second_ctrl->SetValue(k_hh_mm_ss.seconds().count());


  {
    using namespace date;
    int k_h{3};
    std::chrono::hours k_hours{k_h};
    sys_days tt{year{2015}/3/22};
    p_time = tt + k_hours;
  }
}
}  // namespace doodle
