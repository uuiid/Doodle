//
// Created by TD on 2022/7/29.
//

#include "sequence_to_blend_shape.h"
#include <maya/MArgDatabase.h>
#include <maya/MTime.h>
#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MComputation.h>
#include <maya/MFnMesh.h>
#include <maya/MFnDagNode.h>
#include <maya/MDagModifier.h>
#include <maya/MFnIkJoint.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnSet.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MEulerRotation.h>
#include <maya/MQuaternion.h>
#include <maya/MDagPath.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MBoundingBox.h>
#include <maya/MPointArray.h>

namespace doodle {
namespace maya_plug {

namespace sequence_to_blend_shape_ns {
constexpr char startFrame_f[]  = "-sf";
constexpr char startFrame_lf[] = "-startFrame";
constexpr char endFrame_f[]    = "-ef";
constexpr char endFrame_lf[]   = "-endFrame";

constexpr char bindFrame_f[]   = "-bf";
constexpr char bindFrame_lf[]  = "-bindFrame";

constexpr char parent_f[]      = "-p";
constexpr char parent_lf[]     = "-parent";
MSyntax syntax() {
  MSyntax syntax{};
  syntax.addFlag(startFrame_f, startFrame_lf, MSyntax::kTime);
  syntax.addFlag(endFrame_f, endFrame_lf, MSyntax::kTime);
  syntax.addFlag(bindFrame_f, bindFrame_lf, MSyntax::kTime);
  syntax.addFlag(parent_f, parent_lf, MSyntax::kString);

  /// \brief 选中的物体
  syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList, 1);

  syntax.enableEdit(false);
  syntax.enableQuery(false);

  return syntax;
}

}  // namespace sequence_to_blend_shape_ns

class sequence_to_blend_shape::impl {
 public:
  std::int32_t startFrame_p{0};
  std::int32_t endFrame_p{120};
  MObject parent_tran;

  MSelectionList select_list;

  MObjectArray create_mesh_list{};
  MPointArray create_point_list{};
};

sequence_to_blend_shape::sequence_to_blend_shape()
    : p_i(std::make_unique<impl>()) {
}
void sequence_to_blend_shape::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgDatabase k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(sequence_to_blend_shape_ns::startFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ns::startFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->startFrame_p = l_value.value();
  } else {
    p_i->startFrame_p = MAnimControl::minTime().value();
  }
  if (k_prase.isFlagSet(sequence_to_blend_shape_ns::endFrame_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ns::endFrame_f, 0, l_value);
    DOODLE_CHICK(k_s);
    p_i->endFrame_p = l_value.value();
  } else {
    p_i->endFrame_p = MAnimControl::maxTime().value();
  }
  chick_true<doodle_error>(p_i->startFrame_p < p_i->endFrame_p,
                           DOODLE_LOC, "开始帧 {} 大于结束帧 {}",
                           p_i->startFrame_p, p_i->endFrame_p);

  if (k_prase.isFlagSet(sequence_to_blend_shape_ns::parent_f, &k_s)) {
    DOODLE_CHICK(k_s);
    MString l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_ns::parent_f, 0, l_value);
    DOODLE_CHICK(k_s);
    MSelectionList l_select{};
    k_s = l_select.add(l_value);
    DOODLE_CHICK(k_s);
    MDagPath l_path;

    k_s = l_select.getDagPath(0, l_path);
    DOODLE_CHICK(k_s);

    p_i->parent_tran = l_path.transform(&k_s);
    DOODLE_CHICK(k_s);
  }

  k_s = k_prase.getObjects(p_i->select_list);
  DOODLE_CHICK(k_s);
  chick_true<doodle_error>(p_i->select_list.length() > 0, DOODLE_LOC, "未获得选中物体");
}

MStatus sequence_to_blend_shape::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  return redoIt();
}
MStatus sequence_to_blend_shape::undoIt() {
  return MStatus::kSuccess;
}
MStatus sequence_to_blend_shape::redoIt() {
  this->create_mesh();
  this->run_blend_shape_comm();
  //  this->create_anim();
  return MStatus();
}
bool sequence_to_blend_shape::isUndoable() const {
  return false;
}
void sequence_to_blend_shape::create_mesh() {
  MDagPath l_path{};
  MStatus l_s{};
  l_s = p_i->select_list.getDagPath(0, l_path);
  DOODLE_CHICK(l_s);

  l_s = l_path.extendToShape();
  DOODLE_CHICK(l_s);

  MObject l_mesh_obj{l_path.node(&l_s)};
  DOODLE_CHICK(l_s);

  MFnMesh l_mesh{};
  MFnMesh l_create_mesh{};

  for (auto i = p_i->startFrame_p;
       i <= p_i->endFrame_p;
       ++i) {
    l_s = MGlobal::viewFrame(i);
    DOODLE_CHICK(l_s);

    DOODLE_LOG_INFO("获取网格 第 {} 帧的数据", i);
    l_s = l_mesh.setObject(l_mesh_obj);
    DOODLE_CHICK(l_s);

    auto l_create_mesh_obj = l_mesh.duplicate(false, false, &l_s);
    DOODLE_CHICK(l_s);
    auto l_path_tmp = get_dag_path(l_create_mesh_obj);
    center_pivot(l_path_tmp);

    l_s = p_i->create_mesh_list.append(l_create_mesh_obj);
    DOODLE_CHICK(l_s);

    auto l_bind_box = l_mesh.boundingBox(&l_s);
    DOODLE_CHICK(l_s);

    auto l_center = l_bind_box.center();
    l_s           = p_i->create_point_list.append(l_center);
    DOODLE_CHICK(l_s);
  }

  MFnMesh l_obj{};
}
void sequence_to_blend_shape::create_anim() {
}

void sequence_to_blend_shape::center_pivot(MDagPath& in_path) {
  DOODLE_LOG_INFO("开始居中轴");
  MStatus l_s{};
  auto l_tran_path = get_dag_path(in_path.transform());
  MFnTransform l_fn_transform{std::as_const(l_tran_path), &l_s};
  DOODLE_CHICK(l_s);
  DOODLE_LOG_INFO("获取选中物体 {}", get_node_full_name(l_tran_path.node()));

  l_s = in_path.extendToShape();
  DOODLE_CHICK(l_s);
  DOODLE_LOG_INFO("获取网格体 {}", get_node_full_name(in_path.node()));

  /// 获取变换
  //  auto l_mat        = l_fn_transform.transformationMatrix(&l_s);

  /// \brief 冻结座标轴
  /// \brief 获取变换
  const auto l_tran = l_fn_transform.getTranslation(MSpace::kWorld, &l_s);
  MQuaternion l_rot{};
  l_fn_transform.getRotation(l_rot, MSpace::kTransform);
  DOODLE_CHICK(l_s);
  /// \brief 清除变换
  l_s = l_fn_transform.setTranslation({}, MSpace::kWorld);
  DOODLE_CHICK(l_s);
  /// \brief 变换网格体
  for (MItMeshVertex l_it_mesh_vertex{std::as_const(in_path), MObject::kNullObj, &l_s};
       l_s && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    l_s = l_it_mesh_vertex.translateBy(l_tran, MSpace::kWorld);
    DOODLE_CHICK(l_s);
  }
  DOODLE_CHICK(l_s);

  /// \brief 居中座标轴
  MFnMesh l_mesh{std::as_const(in_path), &l_s};
  DOODLE_CHICK(l_s);
  auto l_box = l_mesh.boundingBox(&l_s);
  DOODLE_CHICK(l_s);

  auto l_center = l_box.center();
  DOODLE_LOG_INFO("获取中心 {}", l_center);
  l_s = l_fn_transform.setScalePivot(l_center, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
  l_s = l_fn_transform.setRotatePivot(l_center, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
  DOODLE_LOG_INFO("完成");
}

void sequence_to_blend_shape::run_blend_shape_comm() {
  MDagPath l_path{};
  MStatus l_s{};
  l_s = p_i->select_list.getDagPath(0, l_path);
  DOODLE_CHICK(l_s);

  std::vector<std::string> l_names{};
  for (int l_i = 0; l_i < p_i->create_mesh_list.length(); ++l_i) {
    l_names.emplace_back(get_node_full_name(p_i->create_mesh_list[l_i]));
  }

  auto l_comm = fmt::format("blendShape -ib {} {};", get_node_full_name(l_path.node()), fmt::join(l_names, " "));
  DOODLE_LOG_INFO("run {}", l_comm);
  l_s = MGlobal::executeCommand(d_str{l_comm}, false, false);
  DOODLE_CHICK(l_s);
}

sequence_to_blend_shape::~sequence_to_blend_shape() = default;

}  // namespace maya_plug
}  // namespace doodle
