//
// Created by TD on 2022/5/9.
//

#include "replace_rig_file_command.h"

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include <maya_plug/data/reference_file.h>

#include <maya/MArgDatabase.h>
#include <nlohmann/json.hpp>

namespace doodle::maya_plug {

namespace replace_rig_file_command_ns {

constexpr char json[]      = "-j";
constexpr char json_long[] = "-jsondata";

MSyntax replace_rig_file_syntax() {
  MSyntax l_syntax{};
  //  l_syntax.addFlag(replace_rig_file_command_ns::json, replace_rig_file_command_ns::json_long, MSyntax::kString);
  return l_syntax;
}

}  // namespace replace_rig_file_command_ns

MStatus replace_rig_file_command::doIt(const MArgList& in_arg_list) {
  for (auto&& [e_, ass, re] : g_reg()->view<assets_file, redirection_path_info>().each()) {
    for (auto&& [e, ref] : g_reg()->view<reference_file>().each()) {
      if (ass.path_attr().filename() == ref.get_path().filename()) {
        ref.replace_file(make_handle(e_));
      }
    }
  }

  return MStatus{};
}
void replace_rig_file_command::get_args(const MArgList& in_arg_list) {
  MStatus l_s{};
  MArgDatabase l_arg{syntax(), in_arg_list, &l_s};
  DOODLE_MAYA_CHICK(l_s);

  if (l_arg.isFlagSet(replace_rig_file_command_ns::json, &l_s)) {
    DOODLE_MAYA_CHICK(l_s);
    MString l_json_str{};
    l_s = l_arg.getFlagArgument(replace_rig_file_command_ns::json, 0, l_json_str);
    DOODLE_MAYA_CHICK(l_s);
    auto l_json = nlohmann::json::parse(l_json_str.asUTF8());
  }
}
}  // namespace doodle::maya_plug
