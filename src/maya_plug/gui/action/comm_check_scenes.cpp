//
// Created by TD on 2021/11/2.
//

#include "comm_check_scenes.h"

#include <doodle_core/core/core_set.h>
#include <doodle_core/logger/logger.h>
// #include <doodle_lib/lib_warp/imgui_warp.h>

#include <maya/MCallbackIdArray.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MMessage.h>
#include <maya/MSelectionList.h>

namespace doodle::maya_plug {

comm_check_scenes::comm_check_scenes()
    : p_unlock_normal(),
      p_duplicate_name(),
      p_multilateral_surface(),
      p_uv_set(),
      p_err_1(),
      p_err_2(),
      p_err_3(),
      p_err_4() {
  title_name_ = std::string{name};
}
MStatus comm_check_scenes::run_maya_py_script(const std::string& in_script) {
  MString l_script{};
  l_script.setUTF8(in_script.c_str());
  auto k_s = MGlobal::executePythonCommand(l_script);
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  return MStatus::kSuccess;
}
MStatus comm_check_scenes::unlock_normal() {
  MStatus k_s{};

  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  MSelectionList k_select{};
  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_s = k_iter.getPath(k_dag_path);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    if (!k_dag_path.hasFn(MFn::kMesh)) DOODLE_LOG_INFO("错误的类型");

    /// 开始软化边
    k_s = MGlobal::select(k_dag_path, MObject::kNullObj, MGlobal::kReplaceList);

    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    k_s = run_maya_py_script(R"(import maya.cmds
maya.cmds.polyNormalPerVertex(unFreezeNormal=True)
maya.cmds.polySoftEdge(angle=180, constructionHistory=True)
    )");
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    p_unlock_normal &= (k_s != MStatus::kSuccess);
  }
  k_s = MGlobal::clearSelectionList();
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  p_unlock_normal = (k_s == MStatus::kSuccess);
  return k_s;
}

MStatus comm_check_scenes::duplicate_namel(bool use_select) {
  MStatus k_s{};
  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};
  using m_pair = std::pair<MDagPath, MString>;
  std::multimap<std::string, MDagPath> k_name_list;
  MSelectionList k_select{};
  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_s = k_iter.getPath(k_dag_path);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    auto k_name = k_dag_node.name(&k_s);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    k_name_list.insert({std::string{k_name.asUTF8()}, k_dag_path});
  }
  for (auto& i : k_name_list) {
    if (k_name_list.count(i.first) > 1) {
      p_duplicate_name |= true;
      k_select.add(i.second, MObject::kNullObj, true);
    }
  }
  if (use_select) {
    k_s = MGlobal::setActiveSelectionList(k_select);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
  }

  return k_s;
}

MStatus comm_check_scenes::multilateral_surface(bool use_select) {
  MStatus k_s{};
  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};
  MSelectionList k_select{};

  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_s = k_iter.getPath(k_dag_path);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    MItMeshPolygon k_iter_poly{k_dag_path, MObject::kNullObj, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    std::uint32_t k_face_num{};
    for (; !k_iter_poly.isDone(); k_iter_poly.next()) {
      k_face_num = k_iter_poly.polygonVertexCount(&k_s);
      CHECK_MSTATUS_AND_RETURN_IT(k_s);
      // std::cout << k_iter_poly.getPoints() << std::endl;
      if (k_face_num > 4) {
        p_multilateral_surface = true;
        k_s                    = k_select.add(k_dag_path, k_iter_poly.currentItem());
        CHECK_MSTATUS_AND_RETURN_IT(k_s);
      }
    }
  }
  if (use_select) MGlobal::setActiveSelectionList(k_select);

  return k_s;
}

MStatus comm_check_scenes::uv_set(bool use_select) {
  MStatus k_s{};
  MItDag k_iter{MItDag::kDepthFirst, MFn::kMesh, &k_s};
  MSelectionList k_select{};

  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_s = k_iter.getPath(k_dag_path);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    MFnDagNode k_dag_node{k_dag_path, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    if (!k_dag_path.hasFn(MFn::kMesh)) DOODLE_LOG_INFO("错误的类型");

    MFnMesh k_mesh{k_dag_path, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    if (k_mesh.numUVSets(&k_s) > 1) {
      p_uv_set = true;
      k_s      = k_select.add(k_dag_path);
      CHECK_MSTATUS_AND_RETURN_IT(k_s);
    }
  }
  if (use_select) {
    MGlobal::setActiveSelectionList(k_select);
  }
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_1() {
  MStatus k_s{};
  /// 获得所有的大纲视图， 将选择回调设置为空
  k_s     = run_maya_py_script(R"(import pymel.core
import re
for p in pymel.core.lsUI(panels=True):
    if re.findall("outlinerPanel",p.name()):
        pymel.core.outlinerEditor(p,edit=True, selectCommand="")
  )");
  p_err_1 = (k_s == MStatus::kSuccess);
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_2() {
  MStatus k_s{};
  /// 获取Maya中的所有模型编辑器并重置 editorChanged 事件
  k_s     = run_maya_py_script(R"(import pymel.core
for item in pymel.core.lsUI(editors=True):
   if isinstance(item, pymel.core.ui.ModelEditor):
       pymel.core.modelEditor(item, edit=True, editorChanged="")
  )");
  p_err_2 = (k_s == MStatus::kSuccess);
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_3() {
  MStatus k_s{};
  /// 获取Maya中的所有模型编辑器并重置 editorChanged 事件
  k_s     = run_maya_py_script(R"(import pymel.core
for item in pymel.core.lsUI(editors=True):
   if isinstance(item, pymel.core.ui.ModelEditor):
       pymel.core.modelEditor(item, edit=True, editorChanged="")
  )");
  p_err_3 = (k_s == MStatus::kSuccess);
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_4() {
  MStatus k_s{};
  /// 去除无用的未知插件需求和去除贼健康问题
  k_s = run_maya_py_script(R"doodle_script(import pymel.core
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
  )doodle_script");
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

  MItDependencyNodes k_iter{MFn::kScript, &k_s};
  CHECK_MSTATUS_AND_RETURN_IT(k_s);
  MObject k_node{};
  std::vector<MObject> k_obj_set{};
  for (; !k_iter.isDone(); k_iter.next()) {
    MDagPath k_dag_path{};

    k_node = k_iter.thisNode(&k_s);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    MFnDependencyNode k_dag_node{k_node, &k_s};
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    if (k_dag_node.name() == "vaccine_gene" || k_dag_node.name() == "breed_gene") {
      k_obj_set.push_back(k_dag_node.object());
    }
  }
  for (auto k_obj : k_obj_set) {
    k_s = MGlobal::deleteNode(k_obj);
    CHECK_MSTATUS_AND_RETURN_IT(k_s);
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

  p_err_4 = (k_s == MStatus::kSuccess);
  return MStatus::kSuccess;
}

bool comm_check_scenes::render() {
  if (imgui::Button("检查所有")) {
    DOODLE_LOG_INFO("开始解锁法线");
    unlock_normal();
    DOODLE_LOG_INFO("重复名称检查");
    duplicate_namel(false);
    DOODLE_LOG_INFO("开始检查多边面");
    multilateral_surface(false);
    DOODLE_LOG_INFO("开始检查uv集");
    uv_set(false);
    DOODLE_LOG_INFO("开始去除大纲错误");
    err_1();
    DOODLE_LOG_INFO("开始去除 onModelChange3dc 错误");
    err_2();
    DOODLE_LOG_INFO("开始去除 CgAbBlastPanelOptChangeCallback 错误");
    err_3();
    DOODLE_LOG_INFO("开始贼健康问题");
    err_4();
  }
  dear::Text(fmt::format("解锁法线 {}", p_unlock_normal));
  dear::Text(fmt::format("重复名称 {}", p_duplicate_name));
  imgui::SameLine();
  if (imgui::Button("选择重复物体")) {
    MStatus k_s{};
    k_s = duplicate_namel(true);
    CHECK_MSTATUS(k_s);
  }
  dear::Text(fmt::format("多边面 {}", p_multilateral_surface));
  imgui::SameLine();
  if (imgui::Button("选择多边面")) {
    MStatus k_s{};
    k_s = multilateral_surface(true);
    CHECK_MSTATUS(k_s);
  }
  dear::Text(fmt::format("多个uv集 {}", p_uv_set));
  imgui::SameLine();
  if (imgui::Button("选择多uv集")) {
    MStatus k_s{};
    k_s = uv_set(true);
    CHECK_MSTATUS(k_s);
  }
  dear::Text(fmt::format("错误 (1) {}", p_err_1));
  dear::HelpMarker{"大纲错误"};

  dear::Text(fmt::format("错误 (2) {}", p_err_2));
  dear::HelpMarker{"onModelChange3dc 错误"};

  dear::Text(fmt::format("错误 (3) {}", p_err_3));
  dear::HelpMarker{"CgAbBlastPanelOptChangeCallback 错误"};

  dear::Text(fmt::format("错误 (4) {}", p_err_4));
  dear::HelpMarker{"贼健康问题"};

  // dear::Disabled{!p_duplicate_name} && [&]() {
  // };
  return open;
}
const std::string& comm_check_scenes::title() const { return title_name_; }

}  // namespace doodle::maya_plug
