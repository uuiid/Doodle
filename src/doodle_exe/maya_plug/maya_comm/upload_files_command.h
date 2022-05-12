//
// Created by TD on 2022/5/12.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

namespace doodle {
namespace maya_plug {

namespace upload_files_command_ns {
MSyntax syntax();
constexpr char name[] = "doodle_upload_files_command";

}  // namespace upload_files_command_ns

class upload_files_command : public TemplateAction<
                                 upload_files_command,
                                 upload_files_command_ns::name,
                                 upload_files_command_ns::syntax> {
 public:
  virtual MStatus doIt(const MArgList& in_list) override;
};

}  // namespace maya_plug
}  // namespace doodle
