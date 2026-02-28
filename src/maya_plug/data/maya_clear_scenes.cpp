//
// Created by TD on 2022/2/28.
//

#include "maya_clear_scenes.h"


#include <maya/MCallbackIdArray.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItSelectionList.h>
#include <maya/MSelectionList.h>

namespace doodle::maya_plug {

maya_clear_scenes::maya_clear_scenes() = default;

bool maya_clear_scenes::unlock_normal() {
  MStatus k_s{};

  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  MSelectionList k_select{};
  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};
    k_s = k_iter.getPath(k_dag_path);
    DOODLE_MAYA_CHICK(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    if (!k_dag_path.hasFn(MFn::kMesh)) display_info("错误的类型");

    /// 开始软化边
    k_s = MGlobal::select(k_dag_path, MObject::kNullObj, MGlobal::kReplaceList);
    DOODLE_MAYA_CHICK(k_s);

    k_s = MGlobal::executePythonCommand(
        d_str{R"(import maya.cmds
maya.cmds.polyNormalPerVertex(unFreezeNormal=True)
maya.cmds.polySoftEdge(angle=180, constructionHistory=True)
    )"},
        false, true
    );
    DOODLE_MAYA_CHICK(k_s);
  }
  k_s = MGlobal::clearSelectionList();
  DOODLE_MAYA_CHICK(k_s);
  return true;
}
bool maya_clear_scenes::duplicate_name(MSelectionList& in_select) {
  MStatus k_s{};
  bool l_r{false};
  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};

  std::multimap<std::string, MDagPath> k_name_list;

  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_s = k_iter.getPath(k_dag_path);
    DOODLE_MAYA_CHICK(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    auto k_name = k_dag_node.name(&k_s);
    DOODLE_MAYA_CHICK(k_s);

    k_name_list.insert({std::string{k_name.toLowerCase().asUTF8()}, k_dag_path});
  }
  for (auto& i : k_name_list) {
    if (k_name_list.count(i.first) > 1) {
      in_select.add(i.second, MObject::kNullObj, true);
      l_r = true;
    }
  }

  return l_r;
}
bool maya_clear_scenes::multilateral_surface(MSelectionList& in_select) {
  MStatus k_s{};
  bool l_r{false};
  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};

  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_s = k_iter.getPath(k_dag_path);
    DOODLE_MAYA_CHICK(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    MItMeshPolygon k_iter_poly{k_dag_path, MObject::kNullObj, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t k_face_num{};
    for (; !k_iter_poly.isDone(); k_iter_poly.next()) {
      k_face_num = k_iter_poly.polygonVertexCount(&k_s);
      DOODLE_MAYA_CHICK(k_s);
      if (k_face_num > 4) {
        k_s = in_select.add(k_dag_path, k_iter_poly.currentItem());
        DOODLE_MAYA_CHICK(k_s);
        l_r = true;
      }
    }
  }

  return l_r;
}
bool maya_clear_scenes::uv_set(MSelectionList& in_select) {
  MStatus k_s{};
  bool l_r{false};
  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};

  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_s = k_iter.getPath(k_dag_path);
    DOODLE_MAYA_CHICK(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    if (!k_dag_path.hasFn(MFn::kMesh)) display_info("错误的类型");

    MFnMesh k_mesh{k_dag_path, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    if (k_mesh.numUVSets(&k_s) > 1) {
      k_s = in_select.add(k_dag_path);
      DOODLE_MAYA_CHICK(k_s);
      l_r = true;
    }
  }

  return l_r;
}
bool maya_clear_scenes::err_1() {
  bool l_r{false};
  MStatus k_s{};
  k_s = MGlobal::executePythonCommand(
      d_str{R"(import pymel.core
import re
for p in pymel.core.lsUI(panels=True):
    if re.findall("outlinerPanel",p.name()):
        pymel.core.outlinerEditor(p,edit=True, selectCommand="")
)"},
      true
  );
  DOODLE_MAYA_CHICK(k_s);
  return false;
}
bool maya_clear_scenes::err_2() {
  MStatus k_s{};
  /// 获取Maya中的所有模型编辑器并重置 editorChanged 事件
  k_s = MGlobal::executePythonCommand(d_str{R"(import pymel.core
for item in pymel.core.lsUI(editors=True):
   if isinstance(item, pymel.core.ui.ModelEditor):
       pymel.core.modelEditor(item, edit=True, editorChanged="")
  )"});
  DOODLE_MAYA_CHICK(k_s);
  return false;
}
bool maya_clear_scenes::err_3() {
  MStatus k_s{};
  /// 获取Maya中的所有模型编辑器并重置 editorChanged 事件
  k_s = MGlobal::executePythonCommand(d_str{R"(import pymel.core
for item in pymel.core.lsUI(editors=True):
   if isinstance(item, pymel.core.ui.ModelEditor):
       pymel.core.modelEditor(item, edit=True, editorChanged="")
  )"});
  DOODLE_MAYA_CHICK(k_s);
  return false;
}
bool maya_clear_scenes::err_4() { return false; }
std::tuple<bool, MSelectionList> maya_clear_scenes::multilateral_surface_by_select(const MSelectionList& in_select) {
  MStatus k_s{};
  bool l_r{false};

  MSelectionList l_r_select{};
  MItSelectionList l_it_selection_list{in_select, MFn::kMesh, &k_s};
  DOODLE_MAYA_CHICK(k_s);

  for (; !l_it_selection_list.isDone(); l_it_selection_list.next()) {
    MDagPath k_dag_path{};

    k_s = l_it_selection_list.getDagPath(k_dag_path);
    DOODLE_MAYA_CHICK(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    MItMeshPolygon k_iter_poly{k_dag_path, MObject::kNullObj, &k_s};
    DOODLE_MAYA_CHICK(k_s);
    std::uint32_t k_face_num{};
    for (; !k_iter_poly.isDone(); k_iter_poly.next()) {
      k_face_num = k_iter_poly.polygonVertexCount(&k_s);
      DOODLE_MAYA_CHICK(k_s);
      if (k_face_num > 4) {
        k_s = l_r_select.add(k_dag_path, k_iter_poly.currentItem());
        DOODLE_MAYA_CHICK(k_s);
        l_r = true;
      }
    }
  }

  return std::make_tuple(l_r, l_r_select);
}
void maya_clear_scenes::delete_unknown_node() {
  std::vector<MObject> l_node{};
  MStatus l_s{};
  for (MItDependencyNodes l_it{}; !l_it.isDone(); l_it.next()) {
    auto l_api_type = l_it.thisNode(&l_s).apiType();
    DOODLE_MAYA_CHICK(l_s);
    if (l_api_type == MFn::kUnknown || l_api_type == MFn::kUnknownDag || l_api_type == MFn::kUnknownTransform) {
      l_node.emplace_back(l_it.thisNode());
    }
  }

  for (auto&& l_i : l_node) {
    l_s = MGlobal::deleteNode(l_i);
    DOODLE_MAYA_CHICK(l_s);
  }
}

}  // namespace doodle::maya_plug
