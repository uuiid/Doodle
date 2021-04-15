//
// Created by teXiao on 2020/10/19.
//

#pragma once
#include <doodle_GUI/doodle_global.h>

#include <loggerlib/Logger.h>

#include <corelib/core/coreset.h>

namespace doodle {
class SettingWidght : public wxControl {
  wxWindowIDRef p_;

 public:
  explicit SettingWidght(wxWindow* parent, wxWindowID id);
};
};  // namespace doodle
