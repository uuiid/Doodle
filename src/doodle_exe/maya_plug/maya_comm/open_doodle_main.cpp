//
// Created by TD on 2021/12/13.
//

#include "open_doodle_main.h"

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/doodle_app.h>

namespace doodle::maya_plug {
open_doodle_main::open_doodle_main()  = default;

open_doodle_main::~open_doodle_main() = default;

MStatus open_doodle_main::doIt(const MArgList& list) {
  if (doodle_app::Get()->p_done) {
    doodle_app::Get()->loop_begin();
    doodle_app::Get()->p_done = false;
  }
  return MStatus::kSuccess;
}
}  // namespace doodle::maya_plug
