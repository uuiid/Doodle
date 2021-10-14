//
// Created by TD on 2021/10/14.
//

#include "maya_plug_app.h"

#include <maya_plug/gui/maya_windwos.h>

namespace doodle::maya_plug {
base_widget_ptr maya_plug_app::get_main_windows() const {
  return new_object<maya_windwos>();
}

maya_plug_app::maya_plug_app()
    : doodle_app() {
}
}  // namespace doodle::maya_plug