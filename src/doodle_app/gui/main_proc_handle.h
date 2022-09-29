//
// Created by TD on 2022/4/27.
//

#pragma once
#include <doodle_app/doodle_app_fwd.h>

namespace doodle::gui {

class DOODLE_APP_API main_proc_handle : public details::no_copy {
  main_proc_handle();

 public:
  virtual ~main_proc_handle();
  static main_proc_handle& get();

  std::function<void()> win_destroy{[]() {}};
  std::function<void()> win_close{[]() {}};
};

}  // namespace doodle::gui
