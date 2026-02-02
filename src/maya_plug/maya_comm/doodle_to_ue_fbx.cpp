//
// Created by td_main on 2023/9/26.
//

#include "doodle_to_ue_fbx.h"

#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/fbx_write.h>
#include <maya_plug/fmt/fmt_dag_path.h>

#include <maya/MAngle.h>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MArgList.h>
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

namespace {
constexpr char file_path[]     = "-fp";
constexpr char file_path_l[]   = "-file_path";
constexpr char export_anim[]   = "-nam";
constexpr char export_anim_l[] = "-not_export_anim";
constexpr char ascii_fbx[]     = "-asc";
constexpr char ascii_fbx_l[]   = "-ascii_fbx";
constexpr char sim_mesh[]      = "-sm";
constexpr char sim_mesh_l[]    = "-sim_mesh";

}  // namespace
MSyntax doodle_to_ue_fbx_syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag(file_path, file_path_l, MSyntax::kString);
  l_syntax.addFlag(export_anim, export_anim_l, MSyntax::kNoArg);
  l_syntax.addFlag(ascii_fbx, ascii_fbx_l, MSyntax::kNoArg);
  l_syntax.addFlag(sim_mesh, sim_mesh_l, MSyntax::MArgType::kString);
  l_syntax.makeFlagMultiUse(sim_mesh);
  l_syntax.setObjectType(MSyntax::kSelectionList);
  l_syntax.useSelectionAsDefault(true);
  return l_syntax;
}

doodle_to_ue_fbx::doodle_to_ue_fbx() = default;

MStatus doodle_to_ue_fbx::doIt(const MArgList& in_list) {
  MStatus l_statu{};
  MArgDatabase const l_arg_data{syntax(), in_list, &l_statu};
  maya_chick(l_statu);
  MSelectionList l_list{};
  maya_chick(l_arg_data.getObjects(l_list));
  auto l_begin_time = MAnimControl::minTime();
  auto l_end_time   = MAnimControl::maxTime();

  fbx_write l_fbx_write{};
  FSys::path l_path{};
  MStatus l_status{};
  if (l_arg_data.isFlagSet(file_path, &l_status)) {
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    MString l_file_path{};
    l_status = l_arg_data.getFlagArgument(file_path, 0, l_file_path);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    l_path = conv::to_s(l_file_path);
  } else {
    displayError(conv::to_ms(fmt::format("no file path")));
    return MS::kFailure;
  }

  if (l_arg_data.isFlagSet(export_anim, &l_status)) {
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    l_fbx_write.not_export_anim();
  }
  if (l_arg_data.isFlagSet(ascii_fbx, &l_status)) {
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    l_fbx_write.ascii_fbx();
  }

  MSelectionList l_sim_list{};

  if (l_arg_data.isFlagSet(sim_mesh, &l_status)) {
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);

    auto l_count = l_arg_data.numberOfFlagUses(sim_mesh);
    for (auto i = 0; i < l_count; ++i) {
      MArgList l_arg_list{};
      l_status = l_arg_data.getFlagArgumentList(sim_mesh, i, l_arg_list);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      MString l_sim_mesh = l_arg_list.asString(0, &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);

      l_status = l_sim_list.add(l_sim_mesh);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    }
  }
  MAnimControl::setCurrentTime(l_begin_time);

  if (l_list.length() == 1) {
    MDagPath l_cam_dag_path{};
    l_status = l_list.getDagPath(0, l_cam_dag_path);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    if(l_cam_dag_path.hasFn(MFn::kCamera)) {
      l_fbx_write.set_path(l_path);
      l_fbx_write.write(l_cam_dag_path, l_begin_time, l_end_time, 1.78);
      return MS::kSuccess;
    }
  }

  l_fbx_write.set_path(l_path);
  l_fbx_write.write(l_list, l_sim_list, l_begin_time, l_end_time);

  return MS::kSuccess;
}

doodle_to_ue_fbx::~doodle_to_ue_fbx() = default;

}  // namespace doodle::maya_plug
