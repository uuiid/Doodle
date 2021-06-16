//
// Created by TD on 2021/6/11.
//

#include "TimeWidget.h"

#include <Metadata/TimeDuration.h>
#include <core/Util.h>
#include <date/date.h>
#include <wx/spinctrl.h>

namespace doodle {

TimeWidget::TimeWidget(wxWindow* in_parent, TimeDurationPtr in_timePoint)
    : wxDialog(in_parent, wxID_ANY, ConvStr<wxString>("获得时间"), wxDefaultPosition, wxDefaultSize,
               wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxMAXIMIZE_BOX | wxMINIMIZE_BOX | wxSTAY_ON_TOP),
      p_time(std::move(in_timePoint)),
      p_year_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_month_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_day_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_hour_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_minute_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_second_ctrl(new wxSpinCtrl{this, NewControlId()}),
      p_week_ctrl(new wxStaticText{this, NewControlId(), wxEmptyString}),
      p_str_ctrl(new wxStaticText{this, NewControlId(), wxEmptyString}),
      p_button_ok(new wxButton{this, NewControlId(), ConvStr<wxString>("确认")}),
      p_button_cancel(new wxButton{this, NewControlId(), ConvStr<wxString>("取消")}) {
  p_year_ctrl->SetRange(2000, 3000);
  p_month_ctrl->SetRange(1, 12);
  p_day_ctrl->SetRange(-1, 32);
  p_hour_ctrl->SetRange(-1, 24);
  p_minute_ctrl->SetRange(-1, 60);
  p_second_ctrl->SetRange(-1, 60);

  auto layout = new wxBoxSizer{wxOrientation::wxVERTICAL};
  auto k_l_   = wxUtil::labelAndWidget(this, "年: ", p_year_ctrl);
  layout->Add(k_l_, wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this, "月: ", p_month_ctrl);
  layout->Add(k_l_, wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this, "日: ", p_day_ctrl);
  layout->Add(k_l_, wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this, "星期： ", p_week_ctrl);
  layout->Add(k_l_, wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this, "小时: ", p_hour_ctrl);
  layout->Add(k_l_, wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this, "分钟: ", p_minute_ctrl);
  layout->Add(k_l_, wxSizerFlags{1}.Expand());
  k_l_ = wxUtil::labelAndWidget(this, "秒: ", p_second_ctrl);
  layout->Add(k_l_, wxSizerFlags{1}.Expand());
  layout->Add(wxUtil::labelAndWidget(this, "预览: ", p_str_ctrl), wxSizerFlags{1}.Expand());

  ///添加取消和ok按钮
  k_l_ = new wxBoxSizer{wxOrientation::wxHORIZONTAL};
  k_l_->Add(p_button_ok, wxSizerFlags{1}.Expand());
  k_l_->Add(p_button_cancel, wxSizerFlags{1}.Expand());
  layout->Add(k_l_, wxSizerFlags{1}.Expand());

  init();
  bind_fun();

  this->SetSizer(layout);
  layout->SetSizeHints(this);
  Center();
}
void TimeWidget::init() {
  p_year_ctrl->SetValue(p_time->get_year());
  p_month_ctrl->SetValue(p_time->get_month());
  p_day_ctrl->SetValue(p_time->get_day());
  p_hour_ctrl->SetValue(p_time->get_hour());
  p_minute_ctrl->SetValue(p_time->get_minutes());
  p_second_ctrl->SetValue(p_time->get_second());
  set_week_text();
  set_str_text();
}
void TimeWidget::bind_fun() {
  p_year_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& in_event) {
    p_time->set_year(in_event.GetPosition());
    init();
  });
  p_month_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& in_event) {
    auto k_v = in_event.GetPosition();
    p_time->set_month(k_v);
    init();
  });
  p_day_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& in_event) {
    auto k_v = in_event.GetPosition();
    if (k_v <= 0) {
      (*p_time) -= date::days{1};
    } else {
      p_time->set_day(in_event.GetPosition());
    }
    init();
  });
  p_hour_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& in_event) {
    auto k_v = in_event.GetPosition();
    if (k_v < 0)
      *p_time -= std::chrono::hours{1};
    else
      p_time->set_hour(k_v);
    init();
  });
  p_minute_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& in_event) {
    auto k_v = in_event.GetPosition();
    if (k_v < 0)
      *p_time -= std::chrono::minutes{1};
    else
      p_time->set_minutes(k_v);
    init();
  });
  p_second_ctrl->Bind(wxEVT_SPINCTRL, [this](wxSpinEvent& in_event) {
    auto k_v = in_event.GetPosition();
    if (k_v < 0)
      *p_time -= std::chrono::seconds{1};
    else
      p_time->set_second(k_v);
    init();
  });

  p_button_ok->Bind(wxEVT_BUTTON, [this](wxCommandEvent& in_event) {
    this->EndModal(wxID_OK);
  });
  p_button_cancel->Bind(wxEVT_BUTTON, [this](wxCommandEvent& in_event) {
    this->EndModal(wxID_CANCEL);
  });
}

void TimeWidget::set_week_text() {
  p_week_ctrl->SetLabel(ConvStr<wxString>(p_time->getWeek<std::string>()));
}
void TimeWidget::set_str_text() {
  p_str_ctrl->SetLabel(ConvStr<wxString>(p_time->showStr()));
}
TimeDurationPtr TimeWidget::get_time(wxWindow* in_parent, const TimeDurationPtr& in_time) {
  auto k_t           = std::make_shared<TimeDuration>(in_time->getUTCTime());
  auto k_time_widget = new TimeWidget{in_parent, k_t};
  auto t             = k_time_widget->ShowModal();
  if (t == wxID_OK) {
    return k_time_widget->p_time;
  } else {
    return {};
  }
}
}  // namespace doodle
