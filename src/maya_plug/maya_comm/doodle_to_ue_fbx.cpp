//
// Created by td_main on 2023/9/26.
//

#include "doodle_to_ue_fbx.h"

#include <boost/lambda2.hpp>

#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/fbx_write.h>
#include <maya_plug/data/maya_tool.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <fbxsdk.h>
#include <maya/MAngle.h>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMesh.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnTransform.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <maya/MTime.h>
#include <treehh/tree.hh>
namespace doodle {
namespace maya_plug {

namespace {
constexpr char file_path[]   = "-file_path";
constexpr char file_path_l[] = "-fp";
}  // namespace
MSyntax doodle_to_ue_fbx_syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(file_path, file_path_l, MSyntax::kString);
  l_syntax.setObjectType(MSyntax::kSelectionList);
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

doodle_to_ue_fbx::doodle_to_ue_fbx() {}

MStatus doodle_to_ue_fbx::doIt(const MArgList& in_list) {
  MStatus l_statu{};
  MArgDatabase l_arg_data{syntax(), in_list, &l_statu};
  maya_chick(l_statu);
  MSelectionList l_list{};
  maya_chick(l_arg_data.getObjects(l_list));
  auto l_begin_time = MAnimControl::minTime();
  auto l_end_time   = MAnimControl::maxTime();

  fbx_write l_fbx_write{};
  FSys::path l_path{};
  MStatus l_status{};
  if (l_arg_data.isFlagSet(file_path, &l_status)) {
    maya_chick(l_status);
    MString l_file_path{};
    maya_chick(l_arg_data.getFlagArgument(file_path, 0, l_file_path));
    l_path = conv::to_s(l_file_path);
  } else {
    displayError(conv::to_ms(fmt::format("no file path")));
    return MS::kFailure;
  }

  l_fbx_write.write(l_list, l_begin_time, l_end_time, l_path);

  return MS::kSuccess;
}

doodle_to_ue_fbx::~doodle_to_ue_fbx() = default;

}  // namespace maya_plug
}  // namespace doodle