//
// Created by TD on 2022/2/28.
//

#include "maya_clear_scenes.h"

#include <doodle_core/core/core_set.h>
#include <maya/MCallbackIdArray.h>
#include <maya/MDagPath.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>

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

    if (!k_dag_path.hasFn(MFn::kMesh))
      DOODLE_LOG_INFO("错误的类型");

    /// 开始软化边
    k_s = MGlobal::select(k_dag_path, MObject::kNullObj, MGlobal::kReplaceList);
    DOODLE_MAYA_CHICK(k_s);

    k_s = MGlobal::executePythonCommand(d_str{R"(import maya.cmds
maya.cmds.polyNormalPerVertex(unFreezeNormal=True)
maya.cmds.polySoftEdge(angle=180, constructionHistory=True)
    )"},
                                        false, true);
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

    k_name_list.insert({std::string{k_name.asUTF8()}, k_dag_path});
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

    if (!k_dag_path.hasFn(MFn::kMesh))
      DOODLE_LOG_INFO("错误的类型");

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
  k_s = MGlobal::executePythonCommand(d_str{R"(import pymel.core
import re
for p in pymel.core.lsUI(panels=True):
    if re.findall("outlinerPanel",p.name()):
        pymel.core.outlinerEditor(p,edit=True, selectCommand="")
)"},
                                      true);
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
bool maya_clear_scenes::err_4() {
  MStatus k_s{};
  /// 去除无用的未知插件需求和去除贼健康问题
  k_s = MGlobal::executePythonCommand(d_str{R"doodle_script(import pymel.core
not_nu_plug = ["mayaHlK"]
un_know_plugs = pymel.core.unknownPlugin(query=True, list=True)
if un_know_plugs:
    for un_plug in un_know_plugs:
        if un_plug in not_nu_plug:
            continue
        print("Remove unknown plugin {}".format(un_plug))
        pymel.core.unknownPlugin(un_plug, remove=True)
plugs = pymel.core.pluginInfo(query=True, listPlugins=True)
if plugs:
    for plug in plugs:
        if plug in not_nu_plug:
            continue
        print("Cancel {} Automatic loading of plug-in writing".format(plug))
        pymel.core.pluginInfo(plug, edit=True, writeRequires=False)
print("=" * 30 + "clear ok" + "=" * 30)

for job in pymel.core.scriptJob(listJobs=True):
  if job.find("leukocyte.antivirus()") > 0:
    num = re.findall("""\d+""",job)[0]
    print(num)
    pymel.core.scriptJob(kill=int(num),force=True)

if 'leukocyte' in globals():
  del leukocyte
  )doodle_script"});
  DOODLE_MAYA_CHICK(k_s);

  MItDependencyNodes k_iter{MFn::kScript, &k_s};
  DOODLE_MAYA_CHICK(k_s);
  MObject k_node{};
  std::vector<MObject> k_obj_set{};
  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_node = k_iter.thisNode(&k_s);
    DOODLE_MAYA_CHICK(k_s);

    MFnDependencyNode k_dag_node{k_node, &k_s};
    DOODLE_MAYA_CHICK(k_s);

    if (k_dag_node.name() == "vaccine_gene" ||
        k_dag_node.name() == "breed_gene") {
      k_obj_set.push_back(k_dag_node.object());
    }
  }
  for (auto k_obj : k_obj_set) {
    k_s = MGlobal::deleteNode(k_obj);
    DOODLE_MAYA_CHICK(k_s);
  }

  auto k_maya_script = doodle::win::get_pwd() / "maya" / "scripts";
  if (FSys::exists(k_maya_script)) {
    auto k_user = k_maya_script / "userSetup.py";
    auto k_var  = k_maya_script / "vaccine.py";
    auto k_varc = k_maya_script / "vaccine.pyc";
    MString k_str{};
    if (FSys::exists(k_user)) {
      DOODLE_LOG_INFO("删除 {}", k_user);
      FSys::remove(k_user);
    }
    if (FSys::exists(k_var)) {
      DOODLE_LOG_INFO("删除 {}", k_var);
      FSys::remove(k_var);
    }
    if (FSys::exists(k_varc)) {
      DOODLE_LOG_INFO("删除 {}", k_varc);
      FSys::remove(k_varc);
    }
  }

  return false;
}
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

}  // namespace doodle::maya_plug
