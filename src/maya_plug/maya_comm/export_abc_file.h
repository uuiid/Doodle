//
// Created by TD on 24-5-10.
//

#pragma once
#include <maya_plug/main/maya_plug_fwd.h>

namespace doodle::maya_plug {
namespace export_abc_file_ns {
constexpr char doodle_export_abc_file[]{"doodle_export_abc_file"};
}  // namespace export_abc_file_ns
MSyntax export_abc_file_syntax();
class export_abc_file
    : public TemplateAction<export_abc_file, export_abc_file_ns::doodle_export_abc_file, export_abc_file_syntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};
}  // namespace doodle::maya_plug
