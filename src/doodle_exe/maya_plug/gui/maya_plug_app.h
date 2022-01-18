//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_lib/doodle_app.h>

namespace doodle::maya_plug {
class maya_plug_app : public doodle_app {
 protected:
 private:
  virtual void load_windows() override;

 public:
  maya_plug_app();
  virtual void hide_windows() override;
};
}  // namespace doodle::maya_plug
