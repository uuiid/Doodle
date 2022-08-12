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
#include <maya/MTransformationMatrix.h>
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

  MObject bind_obj;
  MDagPath bind_path;
  MPoint bind_center;
  MMatrix bind_matrix;
  MObject blend_shape_obj;
  MDagModifier dg_modidier;
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
  this->create_anim();
  return MStatus::kSuccess;
}
bool sequence_to_blend_shape::isUndoable() const {
  return false;
}
void sequence_to_blend_shape::create_mesh() {
  MDagPath l_path{};
  MStatus l_s{};
  l_s = p_i->select_list.getDagPath(0, l_path);
  DOODLE_CHICK(l_s);
  MFnTransform l_transform{l_path, &l_s};
  DOODLE_CHICK(l_s);

  MObject l_mesh_obj{l_path.node(&l_s)};
  DOODLE_CHICK(l_s);

  MFnMesh l_mesh{};
  MFnMesh l_create_mesh{};
  MFnDagNode l_tran_node{l_path, &l_s};
  DOODLE_CHICK(l_s);

  {  /// \brief 获取bind obj
    auto i = p_i->startFrame_p;
    l_s    = MGlobal::viewFrame(i);
    DOODLE_CHICK(l_s);

    auto l_create_mesh_obj = l_tran_node.duplicate(false, false, &l_s);
    DOODLE_CHICK(l_s);
    p_i->bind_path = get_dag_path(l_create_mesh_obj);
    //    center_pivot(p_i->bind_path);

    p_i->bind_obj  = l_create_mesh_obj;
    DOODLE_CHICK(l_s);
    p_i->bind_matrix = l_transform.transformationMatrix(&l_s);
    DOODLE_CHICK(l_s);
    l_s = l_mesh.setObject(l_path);
    DOODLE_CHICK(l_s);
    p_i->bind_center = l_mesh.boundingBox(&l_s).center() * p_i->bind_matrix;
    DOODLE_CHICK(l_s);

    add_mat(p_i->bind_obj, l_mesh_obj);
  }

  MFnTransform l_fn_transform_dub{};

  for (auto i = p_i->startFrame_p;
       i <= p_i->endFrame_p;
       ++i) {
    l_s = MGlobal::viewFrame(i);
    DOODLE_CHICK(l_s);

    //    DOODLE_LOG_INFO("获取网格 第 {} 帧的数据", i);
    l_s = l_mesh.setObject(l_path);
    DOODLE_CHICK(l_s);

    auto l_create_mesh_obj = l_mesh.duplicate(false, false, &l_s);
    DOODLE_CHICK(l_s);
    /// \brief 获取网格中心
    auto l_bind_box = l_mesh.boundingBox(&l_s);
    DOODLE_CHICK(l_s);

    l_s = l_transform.setObject(get_dag_path(l_path.transform(&l_s)));
    DOODLE_CHICK(l_s);
    auto l_tran = l_transform.transformationMatrix(&l_s);
    DOODLE_CHICK(l_s);
    //    DOODLE_LOG_INFO("网格tran {}", l_tran);

    auto l_center   = l_bind_box.center();
    auto l_center_1 = (l_center * l_tran) - p_i->bind_center;
    l_s             = p_i->create_point_list.append(l_center_1);
    DOODLE_CHICK(l_s);

    /// \brief 冻结网格数据
    auto l_path_tmp = get_dag_path(l_create_mesh_obj);
    //    center_pivot(l_path_tmp);

    l_s             = p_i->create_mesh_list.append(l_create_mesh_obj);
    DOODLE_CHICK(l_s);

    /// \brief 旋转网格数据
    //    l_path_tmp = get_dag_path(l_path_tmp.transform(&l_s));
    //    DOODLE_CHICK(l_s);
    //    l_s = l_fn_transform_dub.setObject(l_path_tmp);
    //    DOODLE_CHICK(l_s);
    //
    //    l_s = l_fn_transform_dub.setTranslation(l_center, MSpace::kWorld);
    //    DOODLE_CHICK(l_s);
  }
}
void sequence_to_blend_shape::create_anim() {
  MStatus l_s{};
  MFnAnimCurve aim{};
  /// \brief 创建 tran 变形

  MPlug plugtx = get_plug(p_i->bind_obj, "tx");
  MPlug plugty = get_plug(p_i->bind_obj, "ty");
  MPlug plugtz = get_plug(p_i->bind_obj, "tz");
  MPlug plugrx = get_plug(p_i->bind_obj, "rx");
  MPlug plugry = get_plug(p_i->bind_obj, "ry");
  MPlug plugrz = get_plug(p_i->bind_obj, "rz");
  MTimeArray l_time{};
#define DOODLE_ADD_ANM_declaration(axis) \
  MDoubleArray l_value_tran_##axis{};

  DOODLE_ADD_ANM_declaration(x);
  DOODLE_ADD_ANM_declaration(y);
  DOODLE_ADD_ANM_declaration(z);

  for (auto i = p_i->startFrame_p;
       i <= p_i->endFrame_p;
       ++i) {
#define DOODLE_ADD_ANM_set(axis) \
  l_value_tran_##axis.append(l_point.axis);
    auto l_point = p_i->create_point_list[i - p_i->startFrame_p];
    l_time.append(MTime{boost::numeric_cast<std::double_t>(i), MTime::uiUnit()});
    DOODLE_ADD_ANM_set(x);
    DOODLE_ADD_ANM_set(y);
    DOODLE_ADD_ANM_set(z);
  }
#define DOODLE_ADD_ANM_set_anm(axis)                                                     \
  aim.create(plugt##axis, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &p_i->dg_modidier); \
  l_s = aim.addKeys(&l_time, &l_value_tran_##axis);                                      \
  DOODLE_CHICK(l_s);

//  DOODLE_ADD_ANM_set_anm(x);
//  DOODLE_ADD_ANM_set_anm(y);
//  DOODLE_ADD_ANM_set_anm(z);
#undef DOODLE_ADD_ANM_declaration
#undef DOODLE_ADD_ANM_set
#undef DOODLE_ADD_ANM_set_anm
  MPlug plug_weight = get_plug(p_i->blend_shape_obj, "weight");

  /// \brief 每个融合变形负责一帧 开始循环每个融合变形
  for (auto j = 0;
       j < p_i->create_mesh_list.length();
       ++j) {
    MDoubleArray l_value_weight{};
    /// \brief 开始循环每一帧
    for (auto i = p_i->startFrame_p;
         i <= p_i->endFrame_p;
         ++i) {
      const auto l_current_index = i - p_i->startFrame_p;
      l_value_weight.append(l_current_index == j ? 1 : 0);
    }
    aim.create(plug_weight[j], MFnAnimCurve::AnimCurveType::kAnimCurveTL, &p_i->dg_modidier);
    l_s = aim.addKeys(&l_time, &l_value_weight);
    DOODLE_CHICK(l_s);
  }

  l_s = p_i->dg_modidier.doIt();
  DOODLE_CHICK(l_s);
}

void sequence_to_blend_shape::center_pivot(MDagPath& in_path) {
  //  DOODLE_LOG_INFO("开始居中轴");
  MStatus l_s{};
  auto l_tran_path = get_dag_path(in_path.transform());
  MFnTransform l_fn_transform{std::as_const(l_tran_path), &l_s};
  DOODLE_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取选中物体 {}", get_node_full_name(l_tran_path.node()));

  l_s = in_path.extendToShape();
  DOODLE_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取网格体 {}", get_node_full_name(in_path.node()));

  /// \brief 冻结座标轴
  /// 获取变换
  auto l_mat = l_fn_transform.transformationMatrix(&l_s);
  /// \brief 获取变换
  //  const auto l_tran = l_fn_transform.getTranslation(MSpace::kWorld, &l_s);

  /// \brief 清除变换
  //  l_s = l_fn_transform.setTranslation({}, MSpace::kWorld);
  l_s        = l_fn_transform.resetFromRestPosition();
  DOODLE_CHICK(l_s);
  /// \brief 变换网格体
  for (MItMeshVertex l_it_mesh_vertex{std::as_const(in_path), MObject::kNullObj, &l_s};
       l_s && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    auto l_point = l_it_mesh_vertex.position(MSpace::kWorld, &l_s);
    DOODLE_CHICK(l_s);
    l_point = l_point * l_mat;
    l_s     = l_it_mesh_vertex.setPosition(l_point);
    //    l_s     = l_it_mesh_vertex.translateBy(l_tran, MSpace::kWorld);
    DOODLE_CHICK(l_s);
  }
  DOODLE_CHICK(l_s);

  /// \brief 居中座标轴
  MFnMesh l_mesh{std::as_const(in_path), &l_s};
  DOODLE_CHICK(l_s);
  auto l_box = l_mesh.boundingBox(&l_s);
  DOODLE_CHICK(l_s);

  auto l_center = l_box.center();
  //  DOODLE_LOG_INFO("获取中心 {}", l_center);
  l_s           = l_fn_transform.setScalePivot(l_center, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
  l_s = l_fn_transform.setRotatePivot(l_center, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
  //  DOODLE_LOG_INFO("完成");
}
void sequence_to_blend_shape::center_pivot(MDagPath& in_path, const MMatrix& in_matrix, const MPoint& in_point) {
  //  DOODLE_LOG_INFO("开始居中轴");
  MStatus l_s{};
  auto l_tran_path = get_dag_path(in_path.transform());
  MFnTransform l_fn_transform{std::as_const(l_tran_path), &l_s};
  DOODLE_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取选中物体 {}", get_node_full_name(l_tran_path.node()));

  /// \brief 冻结座标轴
  /// \brief 清除变换
  l_s = l_fn_transform.resetFromRestPosition();
  DOODLE_CHICK(l_s);
  /// \brief 变换网格体
  for (MItMeshVertex l_it_mesh_vertex{std::as_const(in_path), MObject::kNullObj, &l_s};
       l_s && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    auto l_point = l_it_mesh_vertex.position(MSpace::kWorld, &l_s);
    DOODLE_CHICK(l_s);
    l_point = l_point * in_matrix;
    l_s     = l_it_mesh_vertex.setPosition(l_point);
    //    l_s     = l_it_mesh_vertex.translateBy(l_tran, MSpace::kWorld);
    DOODLE_CHICK(l_s);
  }
  DOODLE_CHICK(l_s);

  /// \brief 居中座标轴
  MFnMesh l_mesh{std::as_const(in_path), &l_s};
  DOODLE_CHICK(l_s);

  l_s = l_fn_transform.setScalePivot(in_point, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
  l_s = l_fn_transform.setRotatePivot(in_point, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
  //  DOODLE_LOG_INFO("完成");
}
void sequence_to_blend_shape::to_work_zero(const MDagPath& in_path) {
  MStatus l_s{};
  MFnTransform l_fn_transform{in_path, &l_s};
  DOODLE_CHICK(l_s);

  MFnMesh l_mesh{in_path, &l_s};
  DOODLE_CHICK(l_s);
  MVector center = l_mesh.boundingBox(&l_s).center();
  DOODLE_CHICK(l_s);
  auto l_matrix = l_fn_transform.transformationMatrix(&l_s);
  DOODLE_CHICK(l_s);

  center *= l_matrix;
  center = -center;
  l_s    = l_fn_transform.setTranslation(center, MSpace::kWorld);
  DOODLE_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取选中物体 {}", get_node_full_name(l_tran_path.node()));
  l_matrix = l_fn_transform.transformationMatrix(&l_s);
  DOODLE_CHICK(l_s);
  /// \brief 冻结座标轴
  /// \brief 清除变换
  l_s = l_fn_transform.resetFromRestPosition();
  DOODLE_CHICK(l_s);
  auto l_path_mesh = in_path;
  l_s              = l_path_mesh.extendToShape();
  DOODLE_CHICK(l_s);
  /// \brief 变换网格体
  for (MItMeshVertex l_it_mesh_vertex{l_path_mesh, MObject::kNullObj, &l_s};
       l_s && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    auto l_point = l_it_mesh_vertex.position(MSpace::kWorld, &l_s);
    DOODLE_CHICK(l_s);
    l_point = l_point * l_matrix;
    l_s     = l_it_mesh_vertex.setPosition(l_point);
    //    l_s     = l_it_mesh_vertex.translateBy(l_tran, MSpace::kWorld);
    DOODLE_CHICK(l_s);
  }
  DOODLE_CHICK(l_s);

  l_s = l_fn_transform.setScalePivot({}, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
  l_s = l_fn_transform.setRotatePivot({}, MSpace::kWorld, false);
  DOODLE_CHICK(l_s);
}
void sequence_to_blend_shape::run_blend_shape_comm() {
  MStatus l_s{};

  std::vector<std::string> l_names{};
  for (int l_i = 0; l_i < p_i->create_mesh_list.length(); ++l_i) {
    l_names.emplace_back(get_node_full_name(p_i->create_mesh_list[l_i]));
  }

  auto l_comm = fmt::format("blendShape {} {};", fmt::join(l_names, " "), get_node_full_name(p_i->bind_obj));
  //  DOODLE_LOG_INFO("run {}", l_comm);
  MStringArray l_r{};
  l_s = MGlobal::executeCommand(d_str{l_comm}, l_r, false, false);
  DOODLE_CHICK(l_s);
  chick_true<doodle_error>(l_r.length() == 1, DOODLE_LOC, "错误的融合变形节点创建");

  MSelectionList l_selection_list{};
  l_s = l_selection_list.add(l_r[0], true);
  DOODLE_CHICK(l_s);
  l_s = l_selection_list.getDependNode(0, p_i->blend_shape_obj);
  DOODLE_CHICK(l_s);

  for (int l_i = 0; l_i < p_i->create_mesh_list.length(); ++l_i) {
    l_s = MGlobal::deleteNode(p_i->create_mesh_list[l_i]);
    DOODLE_CHICK(l_s);
  }
}

sequence_to_blend_shape::~sequence_to_blend_shape() = default;

}  // namespace maya_plug
}  // namespace doodle
