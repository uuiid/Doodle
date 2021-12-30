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

namespace doodle::maya_plug {

class afterimage_comm::impl {
 public:
  MObject p_util_obj;
  MDagModifier p_modifier;
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
  DOODLE_CHICK(k_s);

  MFnDagNode k_node{};
  MObject k_obj{};
  for (MItSelectionList l_it{p_i->p_select_list};
       !l_it.isDone(&k_s);
       l_it.next()) {
    DOODLE_CHICK(k_s);
    k_s = l_it.getDependNode(k_obj);
    DOODLE_CHICK(k_s);
    k_s = k_node.setObject(k_obj);
    DOODLE_CHICK(k_s);

    auto k_copy_obj = k_node.duplicate(false, false, &k_s);
    DOODLE_CHICK(k_s);
    p_i->p_copy_list.add(k_copy_obj);
  }

  k_s = MGlobal::setActiveSelectionList(p_i->p_copy_list);
  DOODLE_CHICK(k_s);

  auto k_name = fmt::format("doodle_{}", "");

  MStringArray k_r{};
  k_s = MGlobal::executeCommand(R"(polyUnite -ch 1 -mergeUVSets 1 -centerPivot;)", k_r);
  DOODLE_CHICK(k_s);

  MSelectionList k_list{};
  k_s = k_list.add(k_r[0], true);

  chick_true<maya_error>(!k_list.isEmpty(&k_s), DOODLE_LOC, "没有找到合并对象");

  k_s = k_list.getDependNode(0, p_i->p_util_obj);

  chick_true<maya_error>(p_i->p_util_obj.hasFn(MFn::kTransform), DOODLE_LOC, " 没有找到符合的 Transform 节点");

  k_s = MGlobal::setActiveSelectionList(k_list);
  DOODLE_CHICK(k_s);

  /// \brief 运行中心枢轴， 清除变换和删除历史
  k_s = MGlobal::executeCommand("CenterPivot;");
  DOODLE_CHICK(k_s);

  /// 将变换转换为中心
  MFnTransform k_tran{};
  k_s = k_tran.setObject(p_i->p_util_obj);
  DOODLE_CHICK(k_s);
  k_s = k_tran.setTranslation({0, 0, 0}, MSpace::Space::kWorld);
  DOODLE_CHICK(k_s);

  k_s = MGlobal::executeCommand("DeleteHistory;");
  DOODLE_CHICK(k_s);

  k_s = MGlobal::executeCommand("makeIdentity -apply true -t 1 -r 1 -s 1 -n 0 -pn 1;");
  DOODLE_CHICK(k_s);

  return MStatus::kSuccess;
}
MStatus afterimage_comm::undoIt() {
  return MStatus::kSuccess;
}
MStatus afterimage_comm::redoIt() {
  return MStatus::kSuccess;
}
bool afterimage_comm::isUndoable() const {
  return false;
}

}  // namespace doodle::maya_plug
