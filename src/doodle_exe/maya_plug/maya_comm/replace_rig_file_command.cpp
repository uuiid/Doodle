//
// Created by TD on 2022/5/9.
//

#include "replace_rig_file_command.h"
namespace doodle::maya_plug {
MSyntax replace_rig_file_command_ns::replace_rig_file_syntax() {
  return MSyntax();
}

MStatus replace_rig_file_command::doIt(const MArgList &) {
  return MStatus{};
}
}  // namespace doodle::maya_plug
