//
// Created by TD on 2021/12/29.
//

#include "afterimage_comm.h"

#include <maya/MDagModifier.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnTransform.h>
#include <maya/MVector.h>
#include <maya/MDagPath.h>

#include <maya_plug/data/maya_tool.h>

namespace doodle::maya_plug {

class afterimage_comm::impl {
 public:
  MObject p_util_obj;
  MSelectionList p_select_list;
  MSelectionList p_copy_list;
};

afterimage_comm::afterimage_comm()
    : p_i(std::make_unique<impl>()) {
}
afterimage_comm::~afterimage_comm() = default;

MStatus afterimage_comm::doIt(const MArgList &) {
  MStatus k_s{};
  k_s = MGlobal::getActiveSelectionList(p_i->p_select_list);
  DOODLE_MAYA_CHICK(k_s);

  MFnDagNode k_node{};
  MObject k_obj{};
  std::vector<std::string> k_name_s{};
  for (MItSelectionList l_it{p_i->p_select_list};
       !l_it.isDone(&k_s);
       l_it.next()) {
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_it.getDependNode(k_obj);
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_node.setObject(k_obj);
    DOODLE_MAYA_CHICK(k_s);

    auto k_copy_obj = k_node.duplicate(false, false, &k_s);
    DOODLE_MAYA_CHICK(k_s);

    /// 将复制节点的材质属性设置好
    add_mat(k_copy_obj, k_obj);

    k_s = p_i->p_copy_list.add(k_copy_obj, true);
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_node.setObject(k_copy_obj);
    DOODLE_MAYA_CHICK(k_s);
    k_name_s.push_back(d_str{k_node.name()});
  }

  MDagPath k_path{};
  MSelectionList k_list{};

  if (p_i->p_select_list.length(&k_s) > 1) {
    DOODLE_MAYA_CHICK(k_s);
    MStringArray k_r{};
    k_s = MGlobal::executeCommand(d_str{fmt::format(R"(polyUnite -ch 1 -mergeUVSets 1 -centerPivot {};)", fmt::join(k_name_s, " "))}, k_r);
    DOODLE_MAYA_CHICK(k_s);

    k_s = k_list.add(k_r[0], true);

    DOODLE_CHICK(!k_list.isEmpty(&k_s), maya_error{"没有找到合并对象"s});

    k_s = k_list.getDependNode(0, p_i->p_util_obj);
    DOODLE_MAYA_CHICK(k_s);
    k_s = k_list.getDagPath(0, k_path);
    DOODLE_MAYA_CHICK(k_s);
    DOODLE_CHICK(p_i->p_util_obj.hasFn(MFn::kTransform), maya_error{" 没有找到符合的 Transform 节点"s});
  } else {
    MObject k_one_obj{};
    k_s = p_i->p_copy_list.getDependNode(0, k_one_obj);
    DOODLE_MAYA_CHICK(k_s);
    MFnDagNode l_dag_node{k_one_obj, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_dag_node.getPath(k_path);
    DOODLE_MAYA_CHICK(k_s);
    k_list.add(k_path);
    DOODLE_MAYA_CHICK(k_s);

    /// \brief 先删除父级变换
    auto k_root = l_dag_node.parent(0, &k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_dag_node.setObject(k_root);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_dag_node.removeChild(k_one_obj);
    DOODLE_MAYA_CHICK(k_s);

    /// \brief 将删除的父级变换为世界级子物体
    k_root = l_dag_node.dagRoot(&k_s);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_dag_node.setObject(k_root);
    DOODLE_MAYA_CHICK(k_s);
    k_s = l_dag_node.addChild(k_one_obj);
    DOODLE_MAYA_CHICK(k_s);
  }

  k_s = MGlobal::setActiveSelectionList(k_list);
  DOODLE_MAYA_CHICK(k_s);

  /// \brief 运行中心枢轴， 清除变换和删除历史
  k_s = MGlobal::executeCommand("CenterPivot;");
  DOODLE_MAYA_CHICK(k_s);

  /// 将变换转换为中心
  MFnTransform k_tran{k_path, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  auto k_tran_vector = MVector{k_tran.scalePivot(MSpace::Space::kWorld, &k_s)};

  DOODLE_MAYA_CHICK(k_s);
  k_s = k_tran.translateBy(-k_tran_vector, MSpace::Space::kWorld);
  DOODLE_MAYA_CHICK(k_s);

  k_s = MGlobal::executeCommand("DeleteHistory;");
  DOODLE_MAYA_CHICK(k_s);

  k_s = MGlobal::executeCommand("makeIdentity -apply true -t 1 -r 1 -s 1 -n 0 -pn 1;");
  DOODLE_MAYA_CHICK(k_s);

  return MStatus::kSuccess;
}
[[maybe_unused]] MStatus afterimage_comm::undoIt() {
  return MStatus::kSuccess;
}
MStatus afterimage_comm::redoIt() {
  return MStatus::kSuccess;
}

}  // namespace doodle::maya_plug
