//
// Created by TD on 2021/10/14.
//

#pragma once
#include <doodle_lib/doodle_app.h>

namespace doodle::maya_plug {
class maya_plug_app : public doodle_app {
 protected:
  base_widget_ptr get_main_windows() const override;

 public:
  maya_plug_app();
};
}  // namespace doodle::maya_plug
