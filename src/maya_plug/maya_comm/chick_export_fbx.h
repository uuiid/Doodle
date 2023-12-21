//
// Created by TD on 2023/12/18.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace chick_bind_pose_export_fbx_ns {
constexpr char doodle_chick_export_fbx[] = "doodle_chick_export_fbx";

}
MSyntax chick_export_fbx_syntax();
class chick_export_fbx
    : public TemplateAction<
          chick_export_fbx, chick_bind_pose_export_fbx_ns::doodle_chick_export_fbx, chick_export_fbx_syntax> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace doodle::maya_plug