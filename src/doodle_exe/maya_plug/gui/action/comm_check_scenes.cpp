//
// Created by TD on 2021/11/2.
//

#include "comm_check_scenes.h"

#include <doodle_lib/lib_warp/imgui_warp.h>
#include <maya/MDagPath.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnMesh.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MSelectionList.h>

namespace doodle::maya_plug {

comm_check_scenes::comm_check_scenes()
    : command_base_tool(),
      p_unlock_normal(),
      p_duplicate_name(),
      p_multilateral_surface(),
      p_uv_set(),
      p_err_1(),
      p_err_2(),
      p_err_3(),
      p_err_4() {
  p_show_str = make_imgui_name(
      this,
      "检查所有",
      "解锁法线"
      "检查重名"
      "检查大于四边面"
      "检查UV集",
      "去除(1)错误",  // (1)大纲
      "去除(2)错误",  // (2)onModelChange3dc
      "去除(3)错误",  // (3)CgAbBlastPanelOptChangeCallback
      "去除(4)错误"   // (4)贼健康
  );
}
MStatus comm_check_scenes::run_maya_py_script(const string& in_script) {
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

    if (!k_dag_path.hasFn(MFn::kMesh))
      DOODLE_LOG_INFO("错误的类型");

    /// 开始软化边
    k_s = MGlobal::select(k_dag_path, MObject::kNullObj, MGlobal::kReplaceList);

    CHECK_MSTATUS_AND_RETURN_IT(k_s);
    k_s = run_maya_py_script(R"(import pymel.core
pymel.core.polyNormalPerVertex(unFreezeNormal=True)
pymel.core.polySoftEdge(angle=180, constructionHistory=True)
    )");
    CHECK_MSTATUS_AND_RETURN_IT(k_s);

    p_unlock_normal &= (k_s != MStatus::kSuccess);
  }
  k_s = MGlobal::clearSelectionList();
  CHECK_MSTATUS_AND_RETURN_IT(k_s);

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

    // auto k_name = k_dag_node.name(&k_s);
    // CHECK_MSTATUS_AND_RETURN_IT(k_s);

    // MItMeshFaceVertex k_iter_face{k_dag_path, MObject::kNullObj, &k_s};
    // CHECK_MSTATUS_AND_RETURN_IT(k_s);
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

    // MFnMesh k_mesh{k_dag_path, &k_s};
    // CHECK_MSTATUS_AND_RETURN_IT(k_s);

    // MItMeshEdge k_iter_edge{k_dag_path, MObject::kNullObj, &k_s};
    // CHECK_MSTATUS_AND_RETURN_IT(k_s);
  }
  if (use_select)
    MGlobal::setActiveSelectionList(k_select);

  return k_s;
}

MStatus comm_check_scenes::uv_set(bool use_select) {
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_1() {
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_2() {
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_3() {
  return MStatus::kSuccess;
}

MStatus comm_check_scenes::err_4() {
  return MStatus::kSuccess;
}

bool comm_check_scenes::render() {
  if (imgui::Button(p_show_str["检查所有"].c_str())) {
    MGlobal::displayInfo(L"开始解锁法线");
    unlock_normal();
    MGlobal::displayInfo(L"重复名称检查");
    duplicate_namel(false);
    MGlobal::displayInfo(L"开始检查多边面");
    multilateral_surface(false);
    MGlobal::displayInfo(L"开始检查uv集");
    uv_set(false);
    MGlobal::displayInfo(L"开始去除大纲错误");
    err_1();
    MGlobal::displayInfo(L"开始去除 onModelChange3dc 错误");
    err_2();
    MGlobal::displayInfo(L"开始去除 CgAbBlastPanelOptChangeCallback 错误");
    err_3();
    MGlobal::displayInfo(L"开始贼健康问题");
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
  dear::Text(fmt::format("多个uv集 {}", p_multilateral_surface));
  dear::Text(fmt::format("错误 (1) {}", p_multilateral_surface));
  imgui::SameLine();
  dear::HelpMarker("(1)", "大纲错误");

  dear::Text(fmt::format("错误 (2) {}", p_multilateral_surface));
  imgui::SameLine();
  dear::HelpMarker("(1)", "onModelChange3dc 错误");

  dear::Text(fmt::format("错误 (3) {}", p_multilateral_surface));
  imgui::SameLine();
  dear::HelpMarker("(1)", "CgAbBlastPanelOptChangeCallback 错误");

  dear::Text(fmt::format("错误 (4) {}", p_multilateral_surface));
  imgui::SameLine();
  dear::HelpMarker("(1)", "贼健康问题");

  // dear::Disabled{!p_duplicate_name} && [&]() {
  // };

  return false;
}

}  // namespace doodle::maya_plug
