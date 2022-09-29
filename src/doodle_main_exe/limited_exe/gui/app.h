//
// Created by TD on 2022/5/27.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/app/doodle_main_app.h>

class limited_app : public doodle::app {
 public:
  using doodle::doodle_main_app::doodle_main_app;

 protected:
  virtual void load_windows() override;
};
