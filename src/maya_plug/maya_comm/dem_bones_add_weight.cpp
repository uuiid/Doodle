//
// Created by TD on 2022/7/1.
//

#include "dem_bones_add_weight.h"
#include <DemBones/DemBonesExt.h>
#include <maya/MArgDatabase.h>
#include <maya/MTime.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MComputation.h>
#include <maya/MFnMesh.h>
#include <maya/MDagModifier.h>
#include <maya/MFnIkJoint.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnSet.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MEulerRotation.h>
#include <maya/MQuaternion.h>
#include <maya/MDagPath.h>
#include <maya_plug/data/dem_bones_ex.h>
#include <maya_plug/data/maya_tool.h>

#include <doodle_core/lib_warp/entt_warp.h>

namespace doodle::maya_plug {
MSyntax dem_bones_add_weight_ns::syntax() {
  return MSyntax();
}

class dem_bones_add_weight::impl {
 public:
  dem_bones_ex dem;
  MObject skin_mesh_obj;
  MObject skin_obj;
};

dem_bones_add_weight::dem_bones_add_weight()
    : p_i(std::make_unique<impl>()) {
}
MStatus dem_bones_add_weight::doIt(const MArgList& in_arg) {
  return MStatus::kSuccess;
}
void dem_bones_add_weight::get_arg(const MArgList& in_arg) {
}
void dem_bones_add_weight::add_weight() {
}
dem_bones_add_weight::~dem_bones_add_weight() = default;
}  // namespace doodle::maya_plug
