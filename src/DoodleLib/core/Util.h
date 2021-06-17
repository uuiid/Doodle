//
// Created by TD on 2021/5/26.
//

#pragma once
#include <DoodleLib/DoodleLib_fwd.h>

namespace doodle::wxUtil {
wxSizer* labelAndWidget(wxWindow* in_parent, const std::string& in_label, wxWindow* in_ctrl);
}  // namespace doodle

