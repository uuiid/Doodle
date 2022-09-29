//
// Created by TD on 2021/12/13.
//

#include "open_doodle_main.h"

// #include <maya/MQtUtil.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/app/doodle_main_app.h>

namespace doodle::maya_plug {
open_doodle_main::open_doodle_main()  = default;

open_doodle_main::~open_doodle_main() = default;

MStatus open_doodle_main::doIt(const MArgList& list) {
  // auto l_o = MQtUtil::nativeWindow(MQtUtil::mainWindow());

  app::Get().begin_loop();
  app::Get().show_windows();
  // app::Get().set_parent(l_o);
  return MStatus::kSuccess;
}
}  // namespace doodle::maya_plug
