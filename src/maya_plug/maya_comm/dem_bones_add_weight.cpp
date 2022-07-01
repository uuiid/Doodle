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
  MSyntax syntax{};
  /// \brief 选中的物体
  syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList, 1);

  return syntax;
}

class dem_bones_add_weight::impl {
 public:
  impl() : dem(g_reg()->ctx().emplace<dem_bones_ex>()) {}

  void init() {
    MStatus k_s;
    MItSelectionList l_it{select_list, MFn::Type::kMesh, &k_s};
    DOODLE_CHICK(k_s);
    for (; !l_it.isDone(&k_s); l_it.next()) {
      k_s = l_it.getDependNode(skin_mesh_obj);
      DOODLE_CHICK(k_s);
    }
    chick_true<doodle_error>(!skin_mesh_obj.isNull() > 0, DOODLE_LOC, "未获得选中物体");
  }
  dem_bones_ex& dem;
  MSelectionList select_list;
  MObject skin_mesh_obj{};
  MObject skin_obj{};
};

dem_bones_add_weight::dem_bones_add_weight()
    : p_i(std::make_unique<impl>()) {
}
MStatus dem_bones_add_weight::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  p_i->init();
  add_weight();

  return MStatus::kSuccess;
}
void dem_bones_add_weight::add_weight() {
  chick_true<doodle_error>(
      !p_i->skin_obj.isNull(),
      DOODLE_LOC,
      "没有找到绑定皮肤簇");
  MStatus k_s{};

  MFnSkinCluster l_skin_cluster{p_i->skin_obj};

  MFnIkJoint l_fn_joint{};
  MDagPath l_path{};

  std::map<std::int32_t, std::int32_t> joins_index{};

  for (int ibone = 0; ibone < p_i->dem.nB; ibone++) {
    auto l_joint = p_i->dem.joins[ibone];
    k_s          = l_fn_joint.setObject(l_joint);
    DOODLE_CHICK(k_s);
    auto l_joint_index = l_skin_cluster.indexForInfluenceObject(l_path, &k_s);
    joins_index[ibone] = l_joint_index;
    DOODLE_CHICK(k_s);
  }
  MFnMesh l_obj{p_i->skin_mesh_obj};
  k_s = l_obj.getPath(l_path);
  DOODLE_CHICK(k_s);
  MItMeshVertex iterMeshVertex{p_i->skin_mesh_obj};
  for (; !iterMeshVertex.isDone(); iterMeshVertex.next()) {
    auto l_v_i = iterMeshVertex.index();
    for (int ibone = 0; ibone < p_i->dem.nB; ibone++) {
      k_s = l_skin_cluster.setWeights(
          l_path,
          iterMeshVertex.currentItem(),
          joins_index[ibone],
          p_i->dem.w.coeff(ibone, l_v_i), false);
      DOODLE_CHICK(k_s);
    }
  }
}
void dem_bones_add_weight::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgDatabase k_prase{syntax(), in_arg};
  k_s = k_prase.getObjects(p_i->select_list);
  DOODLE_CHICK(k_s);
  chick_true<doodle_error>(p_i->select_list.length() > 0, DOODLE_LOC, "未获得选中物体");
}
dem_bones_add_weight::~dem_bones_add_weight() = default;
}  // namespace doodle::maya_plug
