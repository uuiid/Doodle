//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_lib/app/doodle_main_app.h>

namespace doodle::maya_plug {
class maya_plug_app : public app {
 protected:
 public:
  maya_plug_app(const app::in_gui_arg& in_arg);

 private:
  virtual void load_windows() override;

 protected:
  virtual void post_constructor() override;

 public:
 public:
  using app::app;
  virtual void close_windows() override;
};
}  // namespace doodle::maya_plug
