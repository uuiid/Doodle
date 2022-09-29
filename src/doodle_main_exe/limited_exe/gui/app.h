//
// Created by TD on 2022/5/27.
//

#pragma once

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/app/app.h>

class limited_app : public doodle::app {
 public:
  using doodle::app::app;

 protected:
  virtual void load_windows() override;
};
