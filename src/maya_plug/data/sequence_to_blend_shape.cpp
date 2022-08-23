//
// Created by TD on 2022/8/15.
//

#include "sequence_to_blend_shape.h"

#include <maya_plug/data/reference_file.h>
#include <maya_plug/data/maya_file_io.h>

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
#include <maya/MNamespace.h>
#include <maya/MMatrix.h>
#include <maya/MBoundingBox.h>
#include <maya/MDGContextGuard.h>
#include <maya/MDagPathArray.h>
#include <maya/MPointArray.h>
#include <maya/MDataHandle.h>
#include <maya/MFloatPointArray.h>
#include <maya/MMatrixArray.h>

// #define DOODLE_USE_SELECT_MODEL_COPY_AS_BIND_MODEL

namespace doodle {
namespace maya_plug {

class sequence_to_blend_shape::impl {
 public:
  MDagPath parent_tran{};
  MDagPath select_path{};
  MDagPathArray create_mesh_list{};
  MVectorArray create_point_list{};
  MMatrixArray create_matrix_list{};

  MDagPath bind_path{};
  MPoint bind_center{};
  MMatrix bind_matrix{};
  MObject blend_shape_obj{};

  class bounding_box_ {
   public:
    std::double_t x_min;
    std::double_t y_min;
    std::double_t z_min;

    std::double_t x_max;
    std::double_t y_max;
    std::double_t z_max;

    void add_point(const MPoint& in_point) {
    }

    void get_center() const {
    }
  };
};

sequence_to_blend_shape::sequence_to_blend_shape()
    : ptr(std::make_unique<impl>()) {
}

void sequence_to_blend_shape::to_work_zero(const MDagPath& in_path) {
  MStatus l_s{};
  MFnTransform l_fn_transform{in_path, &l_s};
  DOODLE_MAYA_CHICK(l_s);

  MFnMesh l_mesh{in_path, &l_s};
  DOODLE_MAYA_CHICK(l_s);
  MVector center = l_mesh.boundingBox(&l_s).center();
  DOODLE_MAYA_CHICK(l_s);
  auto l_matrix = l_fn_transform.transformationMatrix(&l_s);
  DOODLE_MAYA_CHICK(l_s);

  center *= l_matrix;
  center = -center;
  //  DOODLE_LOG_INFO("设置中心轴 {}", center);
  l_s    = l_fn_transform.setTranslation(center, MSpace::kWorld);
  DOODLE_MAYA_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取选中物体 {}", get_node_full_name(l_tran_path.node()));
  l_matrix = l_fn_transform.transformationMatrix(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  /// \brief 冻结座标轴
  /// \brief 清除变换
  l_s = l_fn_transform.resetFromRestPosition();
  DOODLE_MAYA_CHICK(l_s);
  auto l_path_mesh = in_path;
  l_s              = l_path_mesh.extendToShape();
  DOODLE_MAYA_CHICK(l_s);
  /// \brief 变换网格体
  for (MItMeshVertex l_it_mesh_vertex{l_path_mesh, MObject::kNullObj, &l_s};
       l_s && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    auto l_point = l_it_mesh_vertex.position(MSpace::kWorld, &l_s);
    DOODLE_MAYA_CHICK(l_s);
    l_point = l_point * l_matrix;
    l_s     = l_it_mesh_vertex.setPosition(l_point);
    //    l_s     = l_it_mesh_vertex.translateBy(l_tran, MSpace::kWorld);
    DOODLE_MAYA_CHICK(l_s);
  }
  DOODLE_MAYA_CHICK(l_s);

  l_s = l_fn_transform.setScalePivot({}, MSpace::kWorld, false);
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_fn_transform.setRotatePivot({}, MSpace::kWorld, false);
  DOODLE_MAYA_CHICK(l_s);
}
void sequence_to_blend_shape::parent_attr(const MDagPath& in_path) {
  ptr->parent_tran = in_path;
}
void sequence_to_blend_shape::select_attr(const MDagPath& in_path) {
  ptr->select_path = in_path;
}

void sequence_to_blend_shape::create_bind_mesh() {
  MStatus l_s{};
  MFnTransform l_transform{};
  MFnMesh l_mesh{};
  MFnDagNode l_dag_path{};
#ifdef DOODLE_USE_SELECT_MODEL_COPY_AS_BIND_MODEL
  /// 使用复制模型
  {  /// \brief 设置函数集
    l_s = l_dag_path.setObject(ptr->select_path);
    DOODLE_MAYA_CHICK(l_s);
    /// \brief
    ptr->bind_path = get_dag_path(l_dag_path.duplicate(false, false, &l_s));
    DOODLE_MAYA_CHICK(l_s);
    l_s = l_transform.setObject(ptr->bind_path);
    DOODLE_MAYA_CHICK(l_s);

    DOODLE_MAYA_CHICK(l_s);
    ptr->bind_matrix = l_transform.transformationMatrix(&l_s);
    DOODLE_MAYA_CHICK(l_s);
    l_s = l_mesh.setObject(ptr->bind_path);
    DOODLE_MAYA_CHICK(l_s);
    ptr->bind_center = l_mesh.boundingBox(&l_s).center() * ptr->bind_matrix;
    DOODLE_MAYA_CHICK(l_s);

    to_work_zero(ptr->bind_path);
    copy_mat(ptr->bind_path, ptr->select_path);
  }
#else
  ptr->bind_path = ptr->select_path;
  /// \brief 如果是使用选择的文件就需要做好解除属性锁定的问题
  DOODLE_LOG_INFO("开始解除节点 {} 属性锁定", get_node_name(ptr->bind_path));

  l_s = l_dag_path.setObject(get_dag_path(ptr->bind_path.transform()));
  DOODLE_MAYA_CHICK(l_s);

  const auto& k_size = l_dag_path.attributeCount(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  for (int l_i = 0; l_i < k_size; ++l_i) {
    auto k_attr = l_dag_path.attribute(l_i, &l_s);
    DOODLE_MAYA_CHICK(l_s);
    auto k_plug = l_dag_path.findPlug(k_attr, false, &l_s);
    //    DOODLE_LOG_INFO("开始解锁属性 {}", k_plug.info());
    if (k_plug.isLocked(&l_s)) {
      DOODLE_MAYA_CHICK(l_s);
      l_s = k_plug.setLocked(false);
      DOODLE_MAYA_CHICK(l_s);
    }
  }
#endif
}
void sequence_to_blend_shape::create_blend_shape_mesh() {
  MStatus l_s{};
  MFnTransform l_transform{};
  MFnMesh l_mesh{};
  MFnDagNode l_dag_path{};

  l_s = l_dag_path.setObject(ptr->select_path);
  DOODLE_MAYA_CHICK(l_s);

  auto l_create_mesh_path = get_dag_path(l_dag_path.duplicate(false, false, &l_s));
  DOODLE_MAYA_CHICK(l_s);

  l_s = l_mesh.setObject(l_create_mesh_path);
  DOODLE_MAYA_CHICK(l_s);
  //      l_mesh.create

  /// \brief 获取网格中心
  auto l_bind_box = l_mesh.boundingBox(&l_s);
  DOODLE_MAYA_CHICK(l_s);

  l_s = l_transform.setObject(l_create_mesh_path);
  DOODLE_MAYA_CHICK(l_s);
  auto l_tran = l_transform.transformationMatrix(&l_s);
  DOODLE_MAYA_CHICK(l_s);
  //    DOODLE_LOG_INFO("网格tran {}", l_tran);

  auto l_center = l_bind_box.center() * l_tran;
  l_s           = ptr->create_point_list.append(l_center);
  DOODLE_MAYA_CHICK(l_s);

  /// \brief 移动到世界中心
  to_work_zero(l_create_mesh_path);
  //    center_pivot(l_path_tmp);

  l_s = ptr->create_mesh_list.append(l_create_mesh_path);
  DOODLE_MAYA_CHICK(l_s);
}

void sequence_to_blend_shape::create_blend_shape_mesh(const MDGContextGuard& in_guard, std::size_t in_index) {
  MStatus l_status;
  auto l_mesh_plug = get_plug(ptr->select_path.node(&l_status), "outMesh");
  DOODLE_MAYA_CHICK(l_status);
  auto l_mesh_data_handle = l_mesh_plug.asMDataHandle(&l_status);
  DOODLE_MAYA_CHICK(l_status);
  /// \brief 上下文网格
  MFnMesh l_ctx_mesh{l_mesh_data_handle.asMesh(), &l_status};
  auto l_ctx_mesh_obj = l_mesh_data_handle.asMesh();
  auto l_matrix       = ptr->create_matrix_list[in_index];

  /// 上下文网格中心
  auto l_center       = ptr->create_point_list[in_index];
  DOODLE_MAYA_CHICK(l_status);
  const std::double_t l_tran[4][4]{1, 0, 0, 0,
                                   0, 1, 0, 0,
                                   0, 0, 1, 0,
                                   -l_center.x, -l_center.y, -l_center.z, 1};
  l_matrix *= MMatrix{l_tran};
  l_matrix = MMatrix{l_tran}.inverse();

  MFnMesh l_create_mesh{};
  MFloatPointArray l_vertexArray{};
  MIntArray l_polygonCounts{};
  MIntArray l_polygonConnects{};
  MIntArray l_polygonConnects_tmp{};

  for (MItMeshPolygon l_it_mesh_polygon{l_ctx_mesh_obj, &l_status};
       l_status && !l_it_mesh_polygon.isDone();
       l_it_mesh_polygon.next()) {
    auto l_polygon_count = l_it_mesh_polygon.count(&l_status);
    DOODLE_MAYA_CHICK(l_status);
    DOODLE_MAYA_CHICK(l_polygonCounts.append(l_polygon_count));
    DOODLE_MAYA_CHICK(l_polygonConnects_tmp.clear());
    l_status = l_it_mesh_polygon.getConnectedVertices(l_polygonConnects_tmp);
    DOODLE_MAYA_CHICK(l_status);
    for (int l_i = 0; l_i < l_polygonConnects_tmp.length(); ++l_i) {
      l_polygonConnects.append(l_polygonConnects_tmp[l_i]);
    }
  }
  /// \brief 复制网格
  auto l_create_obj = l_ctx_mesh.duplicate(false, false, &l_status);
  DOODLE_MAYA_CHICK(l_status);
  for (MItMeshVertex l_it_mesh_vertex{l_ctx_mesh_obj, &l_status};
       l_status && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    auto l_point = l_it_mesh_vertex.position(MSpace::kWorld, &l_status);
    DOODLE_MAYA_CHICK(l_status);
    l_point = l_point * l_matrix;
    DOODLE_MAYA_CHICK(l_status);
    DOODLE_MAYA_CHICK(l_vertexArray.append(l_point));
  }

  l_create_mesh.create(l_ctx_mesh.numVertices(),
                       l_ctx_mesh.numPolygons(),
                       l_vertexArray,
                       l_polygonCounts,
                       l_polygonConnects);

  if (!l_create_obj.isNull()) throw_exception(doodle_error{"创建网格出错 {}", get_node_name(ptr->select_path)});
  DOODLE_MAYA_CHICK(ptr->create_point_list.append(l_center));
  DOODLE_MAYA_CHICK(ptr->create_mesh_list.append(get_dag_path(l_create_obj)));
  DOODLE_MAYA_CHICK(l_status);
}

void sequence_to_blend_shape::create_blend_shape() {
  MStatus l_s{};

  std::vector<std::string> l_names{};
  for (int l_i = 0; l_i < ptr->create_mesh_list.length(); ++l_i) {
    l_names.emplace_back(get_node_full_name(ptr->create_mesh_list[l_i]));
  }
#ifdef DOODLE_USE_SELECT_MODEL_COPY_AS_BIND_MODEL
#else
  MSelectionList l_selection_list_delete_history{};
  l_s = l_selection_list_delete_history.add(ptr->select_path);
  DOODLE_MAYA_CHICK(l_s);

  l_s = MGlobal::setActiveSelectionList(l_selection_list_delete_history);
  DOODLE_MAYA_CHICK(l_s);

  l_s = MGlobal::executeCommand("DeleteHistory");
  DOODLE_MAYA_CHICK(l_s);
  to_work_zero(ptr->bind_path);
#endif

  auto l_comm = fmt::format("blendShape {} {};", fmt::join(l_names, " "), get_node_full_name(ptr->bind_path));
  //  DOODLE_LOG_INFO("run {}", l_comm);
  MStringArray l_r{};
  l_s = MGlobal::executeCommand(d_str{l_comm}, l_r, false, false);
  DOODLE_MAYA_CHICK(l_s);
  if (l_r.length() != 1) throw_exception(doodle_error{"错误的融合变形节点创建"s});

  MSelectionList l_selection_list{};
  l_s = l_selection_list.add(l_r[0], true);
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_selection_list.getDependNode(0, ptr->blend_shape_obj);
  DOODLE_MAYA_CHICK(l_s);

  for (int l_i = 0; l_i < ptr->create_mesh_list.length(); ++l_i) {
    auto l_node = ptr->create_mesh_list[l_i].node(&l_s);
    DOODLE_MAYA_CHICK(l_s);
    l_s = MGlobal::deleteNode(l_node);
    DOODLE_MAYA_CHICK(l_s);
  }
}

void sequence_to_blend_shape::create_blend_shape_anim(std::int64_t in_begin_time,
                                                      std::int64_t in_end_time,
                                                      MDagModifier& in_dg_modidier) {
  MStatus l_s{};

  MFnAnimCurve aim{};
  /// \brief 创建 tran 变形

  MPlug plugtx = get_plug(ptr->bind_path.node(&l_s), "tx");
  DOODLE_MAYA_CHICK(l_s);
  MPlug plugty = get_plug(ptr->bind_path.node(&l_s), "ty");
  DOODLE_MAYA_CHICK(l_s);
  MPlug plugtz = get_plug(ptr->bind_path.node(&l_s), "tz");
  DOODLE_MAYA_CHICK(l_s);
  MPlug plugrx = get_plug(ptr->bind_path.node(&l_s), "rx");
  DOODLE_MAYA_CHICK(l_s);
  MPlug plugry = get_plug(ptr->bind_path.node(&l_s), "ry");
  DOODLE_MAYA_CHICK(l_s);
  MPlug plugrz = get_plug(ptr->bind_path.node(&l_s), "rz");
  DOODLE_MAYA_CHICK(l_s);
  MTimeArray l_time{};
#define DOODLE_ADD_ANM_declaration(axis) \
  MDoubleArray l_value_tran_##axis{};

  DOODLE_ADD_ANM_declaration(x);
  DOODLE_ADD_ANM_declaration(y);
  DOODLE_ADD_ANM_declaration(z);

  for (auto i = in_begin_time;
       i <= in_end_time;
       ++i) {
#define DOODLE_ADD_ANM_set(axis) \
  l_value_tran_##axis.append(l_point.axis);
    auto l_point = ptr->create_point_list[i - in_begin_time];
    l_time.append(MTime{boost::numeric_cast<std::double_t>(i), MTime::uiUnit()});
    DOODLE_ADD_ANM_set(x);
    DOODLE_ADD_ANM_set(y);
    DOODLE_ADD_ANM_set(z);
  }
#define DOODLE_ADD_ANM_set_anm(axis)                                                   \
  aim.create(plugt##axis, MFnAnimCurve::AnimCurveType::kAnimCurveTL, &in_dg_modidier); \
  l_s = aim.addKeys(&l_time, &l_value_tran_##axis);                                    \
  DOODLE_MAYA_CHICK(l_s);

  DOODLE_ADD_ANM_set_anm(x);
  DOODLE_ADD_ANM_set_anm(y);
  DOODLE_ADD_ANM_set_anm(z);
#undef DOODLE_ADD_ANM_declaration
#undef DOODLE_ADD_ANM_set
#undef DOODLE_ADD_ANM_set_anm
  MPlug plug_weight = get_plug(ptr->blend_shape_obj, "weight");

  /// \brief 每个融合变形负责一帧 开始循环每个融合变形
  for (auto j = 0;
       j < ptr->create_mesh_list.length();
       ++j) {
    MDoubleArray l_value_weight{};
    /// \brief 开始循环每一帧
    for (auto i = in_begin_time;
         i <= in_end_time;
         ++i) {
      const auto l_current_index = i - in_begin_time;
      l_value_weight.append(l_current_index == j ? 1 : 0);
    }
    aim.create(plug_weight[j], MFnAnimCurve::AnimCurveType::kAnimCurveTL, &in_dg_modidier);
    l_s = aim.addKeys(&l_time, &l_value_weight);
    DOODLE_MAYA_CHICK(l_s);
  }

  l_s = in_dg_modidier.doIt();
  DOODLE_MAYA_CHICK(l_s);
}

void sequence_to_blend_shape::delete_create_blend_shape_mesh() {
}

void sequence_to_blend_shape::delete_bind_mesh() {
#ifdef DOODLE_USE_SELECT_MODEL_COPY_AS_BIND_MODEL
  MStatus l_status{};
  if (ptr->bind_path.isValid(&l_status)) {
    DOODLE_MAYA_CHICK(l_status)
    auto l_node = ptr->bind_path.node(&l_status);
    DOODLE_MAYA_CHICK(l_status);
    l_status = MGlobal::deleteNode(l_node);
    DOODLE_MAYA_CHICK(l_status);
  }
#endif
}
MDagPath& sequence_to_blend_shape::select_attr() {
  return ptr->select_path;
}
void sequence_to_blend_shape::attach_parent() {
  MStatus l_s{};
  if (ptr->parent_tran.isValid(&l_s)) {
    DOODLE_MAYA_CHICK(l_s);
    add_child(ptr->parent_tran, ptr->bind_path);
  }
}

sequence_to_blend_shape::sequence_to_blend_shape(sequence_to_blend_shape&& in) noexcept
    : ptr(std::move(in.ptr)) {
}

sequence_to_blend_shape& sequence_to_blend_shape::operator=(sequence_to_blend_shape&& in) noexcept {
  ptr = std::move(in.ptr);
  return *this;
}
void sequence_to_blend_shape::delete_select_node() {
#ifdef DOODLE_USE_SELECT_MODEL_COPY_AS_BIND_MODEL
  MStatus l_s{};
  auto l_node_select = ptr->select_path.node(&l_s);
  if (!l_node_select.isNull()) {
    l_s = MGlobal::deleteNode(l_node_select);
    DOODLE_MAYA_CHICK(l_s);
  }
#endif
}

sequence_to_blend_shape::~sequence_to_blend_shape() = default;
}  // namespace maya_plug
}  // namespace doodle
