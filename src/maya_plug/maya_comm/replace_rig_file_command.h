//
// Created by TD on 2022/5/9.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {

namespace replace_rig_file_command_ns {
constexpr char replace_rig_file_command_name[] = "doodle_replace_rig_file";
MSyntax replace_rig_file_syntax();
}  // namespace replace_rig_file_command_ns

class replace_rig_file_command
    : public TemplateAction<
          replace_rig_file_command, replace_rig_file_command_ns::replace_rig_file_command_name,
          replace_rig_file_command_ns::replace_rig_file_syntax> {
  void get_args(const MArgList& in_arg_list);

 public:
  MStatus doIt(const MArgList&) override;
};

}  // namespace doodle::maya_plug
