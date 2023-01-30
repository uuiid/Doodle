//
// Created by TD on 2022/4/27.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>

namespace doodle::gui::details {
class DOODLE_APP_API main_proc_handle {
 public:
  main_proc_handle()          = default;
  virtual ~main_proc_handle() = default;

  std::function<void()> win_destroy{[]() {}};
  std::function<void()> win_close{[]() {}};
};
}  // namespace doodle::gui
