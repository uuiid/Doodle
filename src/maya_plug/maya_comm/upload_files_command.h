//
// Created by TD on 2022/5/12.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {

namespace upload_files_command_ns {
MSyntax syntax();
constexpr char name[] = "doodle_upload_files";

}  // namespace upload_files_command_ns

class upload_files_command
    : public TemplateAction<upload_files_command, upload_files_command_ns::name, upload_files_command_ns::syntax> {
 public:
  MStatus doIt(const MArgList& in_list) override;
};

}  // namespace doodle::maya_plug
