//
// Created by TD on 2022/5/9.
//

#include "replace_rig_file_command.h"
//#include <doodle_lib/doodle_lib_all.h>

namespace doodle::maya_plug {
MSyntax replace_rig_file_command_ns::replace_rig_file_syntax() {
  return MSyntax{};
}

MStatus replace_rig_file_command::doIt(const MArgList &) {
  chick_true<doodle_error>(
      project::has_prj(),
      DOODLE_LOC,
      "缺失project上下文");

  return MStatus{};
}
}  // namespace doodle::maya_plug
