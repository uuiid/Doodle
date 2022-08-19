//
// Created by TD on 2022/7/29.
//

#include "sequence_to_blend_shape_comm.h"

#include <maya_plug/data/sequence_to_blend_shape.h>

#include <maya/MArgDatabase.h>
#include <maya/MSelectionList.h>
#include <maya/MAnimControl.h>
#include <maya/MItMeshVertex.h>
#include <maya/MFnMesh.h>
#include <maya/MDagModifier.h>
#include <maya/MFnIkJoint.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MEulerRotation.h>
#include <maya/MQuaternion.h>
#include <maya/MDagPath.h>
#include <maya/MNamespace.h>
#include <maya/MMatrix.h>
#include <maya/MBoundingBox.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>

namespace doodle::maya_plug {

namespace sequence_to_blend_shape_comm_ns {
constexpr char startFrame_f[]  = "-sf";
constexpr char startFrame_lf[] = "-startFrame";
constexpr char endFrame_f[]    = "-ef";
constexpr char endFrame_lf[]   = "-endFrame";

constexpr char parent_f[]      = "-p";
constexpr char parent_lf[]     = "-parent";

constexpr char duplicate_f[]   = "-du";
constexpr char duplicate_lf[]  = "-duplicate";

MSyntax syntax() {
  MSyntax syntax{};
  syntax.addFlag(startFrame_f, startFrame_lf, MSyntax::kTime);
  syntax.addFlag(endFrame_f, endFrame_lf, MSyntax::kTime);
  syntax.addFlag(parent_f, parent_lf, MSyntax::kString);
  syntax.addFlag(duplicate_f, duplicate_lf, MSyntax::kBoolean);

  /// \brief 选中的物体
  syntax.setObjectType(MSyntax::MObjectFormat::kSelectionList, 1);
  syntax.useSelectionAsDefault(true);

  syntax.enableEdit(false);
  syntax.enableQuery(false);

  return syntax;
}

}  // namespace sequence_to_blend_shape_comm_ns

class sequence_to_blend_shape_comm::impl {
 public:
  std::int32_t startFrame_p{0};
  std::int32_t endFrame_p{120};

  bool duplicate_bind{};

  MSelectionList select_list;

  MDagModifier dg_modidier;

  std::vector<sequence_to_blend_shape> blend_list{};
};

sequence_to_blend_shape_comm::sequence_to_blend_shape_comm()
    : p_i(std::make_unique<impl>()) {
}
void sequence_to_blend_shape_comm::get_arg(const MArgList& in_arg) {
  MStatus k_s;
  MArgDatabase k_prase{syntax(), in_arg};

  if (k_prase.isFlagSet(sequence_to_blend_shape_comm_ns::startFrame_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_comm_ns::startFrame_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->startFrame_p = boost::numeric_cast<std::int32_t>(l_value.value());
  } else {
    p_i->startFrame_p = boost::numeric_cast<std::int32_t>(MAnimControl::minTime().value());
  }
  if (k_prase.isFlagSet(sequence_to_blend_shape_comm_ns::endFrame_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MTime l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_comm_ns::endFrame_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->endFrame_p = boost::numeric_cast<std::int32_t>(l_value.value());
  } else {
    p_i->endFrame_p = boost::numeric_cast<std::int32_t>(MAnimControl::maxTime().value());
  }

  p_i->startFrame_p < p_i->endFrame_p
      ? void()
      : throw_exception(doodle_error{"开始帧 {} 大于结束帧 {}"s,
                                     p_i->startFrame_p < p_i->endFrame_p});
  if (k_prase.isFlagSet(sequence_to_blend_shape_comm_ns::duplicate_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    bool l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_comm_ns::duplicate_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    p_i->duplicate_bind = l_value;
  } else {
    p_i->duplicate_bind = true;
  }

  /// \brief 获取选择物体
  k_s = k_prase.getObjects(p_i->select_list);
  DOODLE_MAYA_CHICK(k_s);
  p_i->select_list.length() > 0 ? void() : throw_exception(doodle_error{"未获得选中物体"});

  /// \brief 生成绑定物体path
  for (auto i = 0;
       i < p_i->select_list.length();
       ++i) {
    sequence_to_blend_shape l_blend_shape{};
    MDagPath l_path{};
    p_i->select_list.getDagPath(i, l_path);
    l_blend_shape.select_attr(l_path);
    p_i->blend_list.emplace_back(std::move(l_blend_shape));
  }

  if (k_prase.isFlagSet(sequence_to_blend_shape_comm_ns::parent_f, &k_s)) {
    DOODLE_MAYA_CHICK(k_s);
    MString l_value{};
    k_s = k_prase.getFlagArgument(sequence_to_blend_shape_comm_ns::parent_f, 0, l_value);
    DOODLE_MAYA_CHICK(k_s);
    MSelectionList l_select{};
    k_s = l_select.add(l_value);
    DOODLE_MAYA_CHICK(k_s);

    MDagPath l_path{};

    k_s = l_select.getDagPath(0, l_path);
    DOODLE_MAYA_CHICK(k_s);

    for (auto&& ctx : p_i->blend_list) {
      ctx.parent_attr(l_path);
    }
  } else {  /// \brief 从本身的名称空间中搜索

    for (auto&& ctx : p_i->blend_list) {
      auto l_namespace = MNamespace::getNamespaceFromName(d_str{get_node_name(ctx.select_attr())}, &k_s);
      MSelectionList l_selection_list{};
      try {
        auto l_select_str = fmt::format("{}:*{}", l_namespace, g_reg()->ctx().at<project_config::base_config>().export_group);
        DOODLE_LOG_INFO("选择 {}", l_select_str);
        k_s = l_selection_list.add(d_str{l_select_str}, true);
        DOODLE_MAYA_CHICK(k_s);
        MDagPath l_path{};
        k_s = l_selection_list.getDagPath(0, l_path);
        DOODLE_MAYA_CHICK(k_s);
        /// \brief 分别为每一个上下文设置父变换
        ctx.parent_attr(l_path);
      } catch (const maya_error& in) {
        DOODLE_LOG_INFO("没有找到ue4组 {}", boost::diagnostic_information(in.what()));
      }
    }
  }
}

MStatus sequence_to_blend_shape_comm::doIt(const MArgList& in_arg) {
  get_arg(in_arg);
  return redoIt();
}
MStatus sequence_to_blend_shape_comm::undoIt() {
  MStatus l_status{};
  for (auto&& ctx : p_i->blend_list) {
    ctx.delete_bind_mesh();
  }
  return MStatus::kSuccess;
}
MStatus sequence_to_blend_shape_comm::redoIt() {
  this->create_mesh();
  this->run_blend_shape_comm();
  this->create_anim();
  this->add_to_parent();

  //  for (auto&& [e, ref] : g_reg()->view<reference_file>().each()) {
  //    maya_file_io::import_reference_file(ref, false);
  //  }

  return MStatus::kSuccess;
}
bool sequence_to_blend_shape_comm::isUndoable() const {
  return true;
}
void sequence_to_blend_shape_comm::create_mesh() {
  MStatus l_s{};

  {  /// \brief 设置时间
    l_s = MGlobal::viewFrame(p_i->startFrame_p);
    DOODLE_MAYA_CHICK(l_s);

    for (auto&& ctx : p_i->blend_list) {
      ctx.create_bind_mesh();
    }
  }

  //  /// \brief 创建属性网格
  //  for (auto i = p_i->startFrame_p;
  //       i <= p_i->endFrame_p;
  //       ++i) {
  //    for (auto&& ctx : p_i->ctx) {
  //      auto l_mesh_obj = p_i->dg_modidier.createNode(d_str{"mesh"s}, MObject::kNullObj, &l_s);
  //      DOODLE_MAYA_CHICK(l_s);
  //      l_s = ctx.create_mesh_list.append(get_dag_path(l_mesh_obj));
  //      DOODLE_MAYA_CHICK(l_s);
  //    }
  //  }
  //  p_i->dg_modidier.doIt();
  //  return;

  for (auto i = p_i->startFrame_p;
       i <= p_i->endFrame_p;
       ++i) {
    /// \brief 设置时间过程
    //    MDGContext l_context{MTime{boost::numeric_cast<std::double_t>(i), MTime::uiUnit()}};
    //    MDGContextGuard l_guard{l_context};
    l_s = MAnimControl::setCurrentTime(MTime{boost::numeric_cast<std::double_t>(i), MTime::uiUnit()});
    DOODLE_MAYA_CHICK(l_s);
    for (auto&& ctx : p_i->blend_list) {
      ctx.create_blend_shape_mesh();
      //    DOODLE_LOG_INFO("获取网格 第 {} 帧的数据", i);
      //      auto l_mesh_data = get_plug(ctx.select_path.node(&l_s), "outMesh");
      //      DOODLE_MAYA_CHICK(l_s);
      //      auto l_mesh_data_handle = l_mesh_data.asMDataHandle(&l_s);
      //      DOODLE_MAYA_CHICK(l_s);
      //      auto l_mesh_obj = l_mesh_data_handle.asMesh();
      //      DOODLE_MAYA_CHICK(l_s);
    }
  }
}
void sequence_to_blend_shape_comm::add_to_parent() {
  for (auto&& ctx : p_i->blend_list) {
    try {
      ctx.attach_parent();
    } catch (const doodle_error& error) {
      DOODLE_LOG_WARN("由于错误 {} 取消附加");
    }
  }
}
void sequence_to_blend_shape_comm::create_anim() {
  for (auto&& ctx : p_i->blend_list) {
    ctx.create_blend_shape_anim(p_i->startFrame_p, p_i->endFrame_p, p_i->dg_modidier);
  }
}

void sequence_to_blend_shape_comm::center_pivot(MDagPath& in_path) {
  //  DOODLE_LOG_INFO("开始居中轴");
  MStatus l_s{};
  auto l_tran_path = get_dag_path(in_path.transform());
  MFnTransform l_fn_transform{std::as_const(l_tran_path), &l_s};
  DOODLE_MAYA_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取选中物体 {}", get_node_full_name(l_tran_path.node()));

  l_s = in_path.extendToShape();
  DOODLE_MAYA_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取网格体 {}", get_node_full_name(in_path.node()));

  /// \brief 冻结座标轴
  /// 获取变换
  auto l_mat = l_fn_transform.transformationMatrix(&l_s);
  /// \brief 获取变换
  //  const auto l_tran = l_fn_transform.getTranslation(MSpace::kWorld, &l_s);

  /// \brief 清除变换
  //  l_s = l_fn_transform.setTranslation({}, MSpace::kWorld);
  l_s        = l_fn_transform.resetFromRestPosition();
  DOODLE_MAYA_CHICK(l_s);
  /// \brief 变换网格体
  for (MItMeshVertex l_it_mesh_vertex{std::as_const(in_path), MObject::kNullObj, &l_s};
       l_s && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    auto l_point = l_it_mesh_vertex.position(MSpace::kWorld, &l_s);
    DOODLE_MAYA_CHICK(l_s);
    l_point = l_point * l_mat;
    l_s     = l_it_mesh_vertex.setPosition(l_point);
    //    l_s     = l_it_mesh_vertex.translateBy(l_tran, MSpace::kWorld);
    DOODLE_MAYA_CHICK(l_s);
  }
  DOODLE_MAYA_CHICK(l_s);

  /// \brief 居中座标轴
  MFnMesh l_mesh{std::as_const(in_path), &l_s};
  DOODLE_MAYA_CHICK(l_s);
  auto l_box = l_mesh.boundingBox(&l_s);
  DOODLE_MAYA_CHICK(l_s);

  auto l_center = l_box.center();
  //  DOODLE_LOG_INFO("获取中心 {}", l_center);
  l_s           = l_fn_transform.setScalePivot(l_center, MSpace::kWorld, false);
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_fn_transform.setRotatePivot(l_center, MSpace::kWorld, false);
  DOODLE_MAYA_CHICK(l_s);
  //  DOODLE_LOG_INFO("完成");
}
void sequence_to_blend_shape_comm::center_pivot(MDagPath& in_path, const MMatrix& in_matrix, const MPoint& in_point) {
  //  DOODLE_LOG_INFO("开始居中轴");
  MStatus l_s{};
  auto l_tran_path = get_dag_path(in_path.transform());
  MFnTransform l_fn_transform{std::as_const(l_tran_path), &l_s};
  DOODLE_MAYA_CHICK(l_s);
  //  DOODLE_LOG_INFO("获取选中物体 {}", get_node_full_name(l_tran_path.node()));

  /// \brief 冻结座标轴
  /// \brief 清除变换
  l_s = l_fn_transform.resetFromRestPosition();
  DOODLE_MAYA_CHICK(l_s);
  /// \brief 变换网格体
  for (MItMeshVertex l_it_mesh_vertex{std::as_const(in_path), MObject::kNullObj, &l_s};
       l_s && !l_it_mesh_vertex.isDone();
       l_it_mesh_vertex.next()) {
    auto l_point = l_it_mesh_vertex.position(MSpace::kWorld, &l_s);
    DOODLE_MAYA_CHICK(l_s);
    l_point = l_point * in_matrix;
    l_s     = l_it_mesh_vertex.setPosition(l_point);
    //    l_s     = l_it_mesh_vertex.translateBy(l_tran, MSpace::kWorld);
    DOODLE_MAYA_CHICK(l_s);
  }
  DOODLE_MAYA_CHICK(l_s);

  /// \brief 居中座标轴
  MFnMesh l_mesh{std::as_const(in_path), &l_s};
  DOODLE_MAYA_CHICK(l_s);

  l_s = l_fn_transform.setScalePivot(in_point, MSpace::kWorld, false);
  DOODLE_MAYA_CHICK(l_s);
  l_s = l_fn_transform.setRotatePivot(in_point, MSpace::kWorld, false);
  DOODLE_MAYA_CHICK(l_s);
  //  DOODLE_LOG_INFO("完成");
}
void sequence_to_blend_shape_comm::run_blend_shape_comm() {
  for (auto&& ctx : p_i->blend_list) {
    ctx.create_blend_shape();
  }
}

sequence_to_blend_shape_comm::~sequence_to_blend_shape_comm() = default;

}  // namespace doodle::maya_plug
