//
// Created by TD on 2022/5/9.
//

#include "replace_rig_file_command.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <maya_plug/data/reference_file.h>
namespace doodle::maya_plug {
MSyntax replace_rig_file_command_ns::replace_rig_file_syntax() {
  return MSyntax{};
}

MStatus replace_rig_file_command::doIt(const MArgList&) {
  for (auto&& [e_, ass, re] : g_reg()->view<assets_file, redirection_path_info>().each()) {
    for (auto&& [e, ref] : g_reg()->view<reference_file>().each()) {
      if (ass.path_attr().filename() == ref.get_path().filename()) {
        ref.replace_file(make_handle(e_));
      }
    }
  }

  return MStatus{};
}
}  // namespace doodle::maya_plug
