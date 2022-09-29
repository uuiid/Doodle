
//
// Created by TD on 2022/1/18.
//

#pragma once
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_app/app/app.h>
namespace doodle {
class DOODLELIB_API main_app : public doodle_main_app {
 public:
  explicit main_app(const in_gui_arg& in_arg);

 protected:
  entt::handle exit_handle;

  void load_windows() override;
};
}  // namespace doodle
