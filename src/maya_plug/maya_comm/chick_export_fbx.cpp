//
// Created by TD on 2023/12/18.
//

#include "chick_export_fbx.h"

#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/fbx_write.h>
#include <maya_plug/fmt/fmt_dag_path.h>

#include <maya/MAngle.h>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>
#include <maya/MEulerRotation.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnMesh.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MObjectArray.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
namespace doodle::maya_plug {

MSyntax chick_export_fbx_syntax() {
  MSyntax l_syntax{};
  l_syntax.setObjectType(MSyntax::kSelectionList);
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

MStatus chick_export_fbx::doIt(const MArgList &in_arg) {
  MStatus l_statu{};

  MArgDatabase const l_arg_data{syntax(), in_arg, &l_statu};
  maya_chick(l_statu);
  MSelectionList l_list{};
  maya_chick(l_arg_data.getObjects(l_list));
  auto l_begin_time = MAnimControl::minTime();
  auto l_end_time   = MAnimControl::maxTime();
  try {
    fbx_write l_fbx_write{};
    l_fbx_write.set_logger(spdlog::default_logger());

    l_fbx_write.write(l_list, l_begin_time, l_end_time);
  } catch (const std::exception& in_error) {
    auto l_str = boost::diagnostic_information(in_error);
    MGlobal::displayError(conv::to_ms(l_str));
    return MS::kFailure;
  }

  return MS::kSuccess;
}
}  // namespace doodle::maya_plug