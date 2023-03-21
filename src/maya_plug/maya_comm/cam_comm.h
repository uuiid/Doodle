//
// Created by TD on 2021/12/13.
//

#pragma once
#include <main/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace {
constexpr char export_camera_command_name[] = "doodle_export_camera";
}  // namespace
namespace details {
MSyntax export_camera_syntax();
}
class export_camera_command
    : public TemplateAction<export_camera_command, export_camera_command_name, details::export_camera_syntax> {
 public:
  MStatus doIt(const MArgList&) override;
};

}  // namespace doodle::maya_plug
