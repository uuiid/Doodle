//
// Created by TD on 2022/9/29.
//

#pragma once

#include <doodle_app/app/app_command.h>

namespace doodle {
class DOODLE_APP_API doodle_main_app : public app_command_base {
 public:
  class in_gui_arg : public app_base::in_app_args {
   public:
    std::int32_t show_enum;
    win::wnd_handle in_parent;
  };

  explicit doodle_main_app(const in_gui_arg& in_arg);

  ~doodle_main_app() override;

  static doodle_main_app& Get();

 protected:
  virtual bool chick_authorization() override;
};
}  // namespace doodle
