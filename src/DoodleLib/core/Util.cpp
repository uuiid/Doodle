//
// Created by TD on 2021/5/26.
//

#include "Util.h"
namespace doodle{
wxSizer* wxUtil::labelAndWidget(wxWindow* in_parent, const std::string& in_label, wxWindow* in_ctrl) {
  auto k_layout = new wxBoxSizer{wxHORIZONTAL};
  auto k_label  = new wxStaticText{in_parent, wxWindow::NewControlId(), ConvStr<wxString>(in_label)};
  k_label->SetMinSize({120, -1});
  k_layout->Add(k_label, wxSizerFlags{0}.Expand());
  k_layout->Add(in_ctrl, wxSizerFlags{1}.Expand());
  return k_layout;
}
}
