

#pragma once
#include <maya_plug/main/maya_plug_fwd.h>
namespace doodle::maya_plug {
namespace bone_weight_ns {
constexpr char dem_bone_weight[] = "doodle_dem_bone_weight";
}
class dem_bone_weight : public TemplateAction<dem_bone_weight, bone_weight_ns::dem_bone_weight> {
 public:
  MStatus doIt(const MArgList& in_arg) override;
};

}  // namespace doodle::maya_plug
