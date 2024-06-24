//
// Created by td_main on 2023/10/16.
//

#include "fbx_write.h"

#include <boost/lambda2.hpp>

#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/data/sequence_to_blend_shape.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>
#include <maya_plug/fmt/fmt_warp.h>

#include <fbxsdk.h>
#include <fmt/std.h>
#include <maya/MAnimControl.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnCamera.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MTime.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MVector.h>
#include <treehh/tree.hh>
namespace doodle::maya_plug {
namespace fbx_write_ns {

struct skin_guard {
  MObject skin_{};
  explicit skin_guard(const MObject& in_sk) : skin_{in_sk} {
    if (!skin_.isNull()) {
      get_plug(skin_, "envelope").setDouble(0.0);
    }
  }
  ~skin_guard() {
    if (!skin_.isNull()) {
      MFnSkinCluster l_fn_skin{skin_};
      get_plug(skin_, "envelope").setDouble(1.0);
    }
  }
};
struct blend_shape_guard {
  std::vector<MObject> blend_shape_list_{};
  explicit blend_shape_guard(const std::vector<MObject>& in_list) : blend_shape_list_{in_list} {
    for (auto& l_blend_shape : blend_shape_list_) {
      get_plug(l_blend_shape, "envelope").setDouble(0.0);
    }
  }
  ~blend_shape_guard() {
    for (auto& l_blend_shape : blend_shape_list_) {
      get_plug(l_blend_shape, "envelope").setDouble(1.0);
    }
  }
};

void fbx_node::build_node() {
  std::call_once(flag_, [&]() { build_data(); });
}
FbxTime::EMode fbx_node::maya_to_fbx_time(MTime::Unit in_value) {
  switch (in_value) {
    case MTime::k25FPS:
      return FbxTime::ePAL;
    case MTime::k24FPS:
      return FbxTime::eFrames24;
    case MTime::k30FPS:
      return FbxTime::eFrames30;
    default:
      return FbxTime::ePAL;
  }
}
void fbx_node::set_node_transform_matrix(const MTransformationMatrix& in_matrix) const {
  MStatus l_status{};
  auto l_loc = in_matrix.getTranslation(MSpace::kWorld, &l_status);
  maya_chick(l_status);
  node->LclTranslation.Set({l_loc.x, l_loc.y, l_loc.z});
  auto l_rot = in_matrix.eulerRotation();
  //  l_rot      = l_rot.boundIt().closestSolution(previous_frame_euler_rotation);

  MAngle l_angle_x{};
  l_angle_x.setUnit(MAngle::kRadians);
  l_angle_x.setValue(l_rot.x);
  MAngle l_angle_y{};
  l_angle_y.setUnit(MAngle::kRadians);
  l_angle_y.setValue(l_rot.y);
  MAngle l_angle_z{};
  l_angle_z.setUnit(MAngle::kRadians);
  l_angle_z.setValue(l_rot.z);

  node->LclRotation.Set({l_angle_x.asDegrees(), l_angle_y.asDegrees(), l_angle_z.asDegrees()});
  std::double_t l_scale[3]{};
  in_matrix.getScale(l_scale, MSpace::kWorld);
  node->LclScaling.Set({l_scale[0], l_scale[1], l_scale[2]});
  node->ScalingMax.Set({});
}

void fbx_node::build_node_transform(MDagPath in_path) const {
  MStatus l_status{};
  node->SetRotationActive(true);
  MFnTransform const l_transform{in_path};
  auto l_rotate_order = l_transform.rotationOrder(&l_status);
  maya_chick(l_status);
  switch (l_rotate_order) {
    case MTransformationMatrix::kXYZ:
      node->RotationOrder.Set(FbxEuler::eOrderXYZ);
      break;
    case MTransformationMatrix::kYZX:
      node->RotationOrder.Set(FbxEuler::eOrderYZX);
      break;
    case MTransformationMatrix::kZXY:
      node->RotationOrder.Set(FbxEuler::eOrderZXY);
      break;
    case MTransformationMatrix::kXZY:
      node->RotationOrder.Set(FbxEuler::eOrderXZY);
      break;
    case MTransformationMatrix::kYXZ:
      node->RotationOrder.Set(FbxEuler::eOrderYXZ);
      break;
    case MTransformationMatrix::kZYX:
      node->RotationOrder.Set(FbxEuler::eOrderZYX);
      break;

    default:
      node->RotationOrder.Set(FbxEuler::eOrderXYZ);
      break;
  }
  node->UpdatePivotsAndLimitsFromProperties();

  set_node_transform_matrix(l_transform.transformation());
}
///

void fbx_node_cam::build_data() {
  fbx_node_transform::build_data();
  auto* l_cam = FbxCamera::Create(node->GetScene(), get_node_name(dag_path).c_str());
  node->SetNodeAttribute(l_cam);
  MFnCamera l_fn_cam{dag_path};
  l_cam->ProjectionType.Set(l_fn_cam.isOrtho() ? FbxCamera::eOrthogonal : FbxCamera::ePerspective);
  std::int32_t l_width{}, l_height{};
  std::double_t l_horizontal_fov{}, l_vertical_fov{};
  l_fn_cam.getPortFieldOfView(l_width, l_height, l_horizontal_fov, l_vertical_fov);
  l_cam->SetAspect(FbxCamera::EAspectRatioMode::eWindowSize, l_width, l_height);
  l_cam->FilmAspectRatio.Set(l_fn_cam.aspectRatio());

  l_cam->SetApertureWidth(l_fn_cam.horizontalFilmAperture());
  l_cam->SetApertureHeight(l_fn_cam.verticalFilmAperture());
  l_cam->SetApertureMode(FbxCamera::eFocalLength);
  l_cam->FocalLength.Set(l_fn_cam.focalLength());
  l_cam->FocusDistance.Set(l_fn_cam.focusDistance());

  l_cam->Position.Set(l_cam->EvaluatePosition());
  //  l_cam->EvaluateUpDirection(l_cam->EvaluatePosition(), l_cam->EvaluateLookAtPosition());
}
void fbx_node_cam::build_animation(const MTime& in_time) {}

///

////
void fbx_node_transform::build_data() {
  build_node_transform(dag_path);
  MFnTransform l_transform{dag_path};
  auto l_attr_null = FbxNull::Create(node->GetScene(), l_transform.name().asChar());
  l_attr_null->Look.Set(FbxNull::eNone);
  node->SetNodeAttribute(l_attr_null);

  if (extra_data_.bind_post->count(dag_path)) {
    previous_frame_euler_rotation = extra_data_.bind_post->at(dag_path).form_matrix.eulerRotation();
  } else {
    previous_frame_euler_rotation = l_transform.transformation().eulerRotation();
  }
}

void fbx_node_transform::build_animation(const MTime& in_time) {
  FbxTime l_fbx_time{};
  l_fbx_time.SetFrame(in_time.value(), maya_to_fbx_time(in_time.unit()));

  auto* l_layer = node->GetScene()->GetCurrentAnimationStack()->GetMember<FbxAnimLayer>();
  MStatus l_status{};
  // tran x
  {
    auto* l_anim_curve = node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_tran_x    = get_plug(dag_path.node(), "translateX").asDouble();
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_tran_x);
    l_anim_curve->KeyModifyEnd();
  }
  // tran y
  {
    auto* l_anim_curve = node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_tran_y    = get_plug(dag_path.node(), "translateY").asDouble();
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_tran_y);
    l_anim_curve->KeyModifyEnd();
  }
  // tran z
  {
    auto* l_anim_curve = node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_tran_z    = get_plug(dag_path.node(), "translateZ").asDouble();
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_tran_z);
    l_anim_curve->KeyModifyEnd();
  }

  {
    auto l_rot_x = get_plug(dag_path.node(), "rotateX").asMAngle(&l_status);
    auto l_rot_y = get_plug(dag_path.node(), "rotateY").asMAngle(&l_status);
    auto l_rot_z = get_plug(dag_path.node(), "rotateZ").asMAngle(&l_status);
    // rot x
    {
      auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
      l_anim_curve->KeyModifyBegin();
      auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
      maya_chick(l_status);
      l_anim_curve->KeySet(l_key_index, l_fbx_time, l_rot_x.asDegrees());
      l_anim_curve->KeyModifyEnd();
    }

    // rot y
    {
      auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
      l_anim_curve->KeyModifyBegin();
      auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
      maya_chick(l_status);
      l_anim_curve->KeySet(l_key_index, l_fbx_time, l_rot_y.asDegrees());
      l_anim_curve->KeyModifyEnd();
    }

    // rot z
    {
      auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
      l_anim_curve->KeyModifyBegin();
      auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
      maya_chick(l_status);
      l_anim_curve->KeySet(l_key_index, l_fbx_time, l_rot_z.asDegrees());
      l_anim_curve->KeyModifyEnd();
    }
  }

  // size x
  {
    auto* l_anim_curve = node->LclScaling.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_size_x    = get_plug(dag_path.node(), "scaleX").asDouble();
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_size_x);
    l_anim_curve->KeyModifyEnd();
  }
  // size y
  {
    auto* l_anim_curve = node->LclScaling.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_size_y    = get_plug(dag_path.node(), "scaleY").asDouble();
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_size_y);
    l_anim_curve->KeyModifyEnd();
  }
  // size z
  {
    auto* l_anim_curve = node->LclScaling.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_size_z    = get_plug(dag_path.node(), "scaleZ").asDouble();
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_size_z);
    l_anim_curve->KeyModifyEnd();
  }
}
////

void fbx_node_mesh::build_data() {
  if (!dag_path.isValid()) return;
  fbx_node_transform::build_data();

  build_mesh();
  // 如果是解算, 只需要构建mesh
  if (is_sim) return;
  build_skin();
  build_blend_shape();
}

void fbx_node_mesh::build_bind_post() {
  // 解算 不需要构建 bindpose
  if (is_sim) return;

  MStatus l_status{};
  auto l_bind_post_obj = get_bind_post();

  if (l_bind_post_obj.isNull() || !l_bind_post_obj.hasFn(MFn::Type::kDagPose)) {
    extra_data_.logger_->log(log_loc(), level::err, "{} 中, 未找到 bind pose 节点", dag_path);
    return;
  }

  if (std::find_if(
          extra_data_.bind_pose_array_->begin(), extra_data_.bind_pose_array_->end(),
          [&](const auto& in_bind_pose) -> bool { return in_bind_pose == l_bind_post_obj; }
      ) != extra_data_.bind_pose_array_->end()) {
    log_info(fmt::format("{} 已经存在, 不进行查找", get_node_name(l_bind_post_obj)));
    return;
  }

  extra_data_.bind_pose_array_->append(l_bind_post_obj);

  auto l_member_list       = get_plug(l_bind_post_obj, "members");
  auto l_world_matrix_list = get_plug(l_bind_post_obj, "worldMatrix");
  auto l_xform_matrix_list = get_plug(l_bind_post_obj, "xformMatrix");
  auto l_world             = get_plug(l_bind_post_obj, "world");
  auto l_global_list       = get_plug(l_bind_post_obj, "global");
  auto l_parent_list       = get_plug(l_bind_post_obj, "parents");
  l_world_matrix_list.evaluateNumElements(&l_status);
  l_xform_matrix_list.evaluateNumElements(&l_status);
  maya_chick(l_status);

  // 缺失bindpose的tran节点和对于的索引
  std::map<MDagPath, std::int32_t, details::cmp_dag> l_tran_map{};

  const auto l_couts = l_member_list.evaluateNumElements(&l_status);
  maya_chick(l_status);
  for (auto i = 0; i < l_couts; ++i) {
    //    bool l_is_global = l_global_list.elementByPhysicalIndex(i, &l_status).asBool();
    //    maya_chick(l_status);
    auto l_member = l_member_list.elementByPhysicalIndex(i, &l_status);
    maya_chick(l_status);
    auto l_member_source = l_member.source(&l_status);
    maya_chick(l_status);
    auto l_node = l_member_source.node(&l_status);
    maya_chick(l_status);
    if (l_node.hasFn(MFn::kDagNode)) {
      MFnDagNode l_fn_node{l_node};
      MDagPath l_path{};
      maya_chick(l_fn_node.getPath(l_path));
      //      auto l_matrix_plug = l_xform_matrix_list.elementByPhysicalIndex(i, &l_status);
      //      maya_chick(l_status);
      //      MObject l_handle{};
      //      maya_chick(l_matrix_plug.getValue(l_handle));
      //      MFnMatrixData l_matrix_data{l_handle};
      //
      //      auto l_matrix = l_matrix_data.transformation(&l_status);
      //      maya_chick(l_status);
      MTransformationMatrix l_world_matrix{};

      if (l_node.hasFn(MFn::Type::kJoint)) {
        auto l_world_matrix_plug = l_world_matrix_list.elementByLogicalIndex(i, &l_status);
        maya_chick(l_status);
        MObject l_world_handle{};
        if (l_world_matrix_plug.getValue(l_world_handle)) {
          const MFnMatrixData l_data{l_world_handle};
          l_world_matrix = l_data.transformation(&l_status);
          maya_chick(l_status);
        } else {
          extra_data_.logger_->log(
              log_loc(), level::err, "正在使用 {} 节点的备用值 bindpose 属性 可能不准确", l_path,
              l_world_matrix_plug.partialName()
          );
          auto l_post_plug = get_plug(l_node, "bindPose");
          MObject l_post_handle{};
          if (l_post_plug.getValue(l_post_handle)) {
            const MFnMatrixData l_data{l_post_handle};
            l_world_matrix = l_data.transformation(&l_status);
            maya_chick(l_status);
          } else {
            throw_exception(doodle_error{
                fmt::format("没有找到 bindpose 属性 {} 的值", conv::to_s(l_world_matrix_plug.partialName()))
            });
          }
        }

      } else if (l_node.hasFn(MFn::Type::kTransform)) {
        auto l_world_matrix_plug = l_world_matrix_list.elementByLogicalIndex(i, &l_status);
        maya_chick(l_status);
        MObject l_world_handle{};
        if (l_world_matrix_plug.getValue(l_world_handle)) {
          const MFnMatrixData l_data{l_world_handle};
          l_world_matrix = l_data.transformation(&l_status);
          maya_chick(l_status);
        } else {
          l_tran_map.emplace(l_path, i);
          extra_data_.logger_->log(
              log_loc(), level::err, "没有找到 节点{} bindpose 属性 {} 的值, 将在第二次中寻找", l_path,
              l_world_matrix_plug.partialName()
          );
        }
      } else {
        throw_exception(doodle_error{"没有找到bindpose, 找绑定 {}", i});
      }

      (*extra_data_.bind_post)[l_path] = {l_world_matrix, l_world_matrix};
    } else {
      log_error(fmt::format("node {} is not dag node", l_member.name()));
      //      throw_exception(doodle_error{"错误的 bindpose, 找绑定 {}", i});
    }
  }

  for (const auto& [l_path, l_index] : l_tran_map) {
    MMatrix l_parent_world_matrix{};
    auto l_parent_path = l_path;
    l_parent_path.pop();
    if (extra_data_.bind_post->contains(l_parent_path)) {
      l_parent_world_matrix = extra_data_.bind_post->at(l_parent_path).world_matrix.asMatrix();
    }
    auto l_xform_matrix_plug = l_xform_matrix_list.elementByLogicalIndex(l_index, &l_status);
    maya_chick(l_status);
    MObject l_xform_handle{};
    if (l_xform_matrix_plug.getValue(l_xform_handle)) {
      const MFnMatrixData l_data{l_xform_handle};
      const auto& l_xform_matrix = l_data.matrix(&l_status);
      maya_chick(l_status);

      auto l_world_matrix              = l_xform_matrix * l_parent_world_matrix;
      (*extra_data_.bind_post)[l_path] = {l_world_matrix, l_xform_matrix};
    } else {
      throw_exception(doodle_error{fmt::format(
          "在二次寻找中没有找到 {} bindpose 属性 {} 的值", l_path, conv::to_s(l_xform_matrix_plug.partialName())
      )});
    }
  }

  std::set<MDagPath, details::cmp_dag> l_all_path{};
  for (auto& l_bp : *extra_data_.bind_post) {
    auto l_parent_path = l_bp.first;
    while (l_parent_path.length() > 0) {
      l_all_path.insert(l_parent_path);
      maya_chick(l_parent_path.pop());
    }
  }
  for (auto&& l_path : l_all_path) {
    if (!extra_data_.bind_post->contains(l_path)) {
      (*extra_data_.bind_post)[l_path] = {MTransformationMatrix::identity, MTransformationMatrix::identity};
    }
  }

  for (auto& l_bp : *extra_data_.bind_post) {
    if (l_bp.first.length() == 1) continue;
    MDagPath l_parent_path{l_bp.first};
    maya_chick(l_parent_path.pop());

    auto l_parent_world_matrix = extra_data_.bind_post->at(l_parent_path).world_matrix;

    MEulerRotation l_joint_rotate{};

    if (l_bp.first.hasFn(MFn::Type::kJoint)) {
      auto l_vector_x = get_plug(l_bp.first.node(), "jointOrientX").asDouble(&l_status);
      maya_chick(l_status);
      auto l_vector_y = get_plug(l_bp.first.node(), "jointOrientY").asDouble(&l_status);
      maya_chick(l_status);
      auto l_vector_z = get_plug(l_bp.first.node(), "jointOrientZ").asDouble(&l_status);
      maya_chick(l_status);
      l_joint_rotate.setValue(l_vector_x, l_vector_y, l_vector_z);
    }
    l_bp.second.form_matrix = l_bp.second.world_matrix.asMatrix() * l_parent_world_matrix.asMatrixInverse();
    //    l_bp.second.form_matrix = l_bp.second.world_matrix.asMatrix() * l_parent_world_matrix.asMatrixInverse() *
    //                              l_joint_rotate.asMatrix().inverse();
    //    l_bp.second.form_matrix = l_bp.second.world_matrix.asMatrix() * l_joint_rotate.asMatrix().inverse() *
    //                              l_parent_world_matrix.asMatrixInverse();
    l_bp.second.form_matrix.rotateBy(l_joint_rotate.inverse(), MSpace::kTransform);
  }
}

void fbx_node_mesh::build_mesh() {
  if (!dag_path.hasFn(MFn::kMesh)) {
    //      log_info(fmt::format("{} is not mesh", get_node_name(in_mesh)));
    return;
  }
  log_info(fmt::format("build mesh {}", dag_path));

  auto l_sk = get_skin_custer();
  skin_guard const l_guard{l_sk};
  blend_shape_guard const l_blend_guard{find_blend_shape()};

  auto l_mesh = dag_path;
  maya_chick(l_mesh.extendToShape());
  MFnMesh l_fn_mesh{l_mesh};
  mesh = FbxMesh::Create(node->GetScene(), "");
  node->SetNodeAttribute(mesh);
  // 顶点
  {
    const auto l_point_count = l_fn_mesh.numVertices();
    mesh->InitControlPoints(l_point_count);
    auto* l_points = mesh->GetControlPoints();
    MPointArray l_m_points{};
    l_fn_mesh.getPoints(l_m_points, MSpace::kObject);
    for (auto i = 0; i < l_point_count; ++i) {
      l_points[i] = FbxVector4{l_m_points[i].x, l_m_points[i].y, l_m_points[i].z, l_m_points[i].w};
    }
  }

  {
    std::vector<std::int32_t> l_mat_ids{};
    auto* l_mat_layer = mesh->CreateElementMaterial();
    auto l_mats       = get_shading_engines(l_mesh);
    if (l_mats.size() == 1) {
      l_mat_layer->SetMappingMode(fbxsdk::FbxLayerElement::eAllSame);
      l_mat_layer->SetReferenceMode(fbxsdk::FbxLayerElement::eIndexToDirect);
      l_mat_layer->GetIndexArray().Add(0);
      FbxSurfaceLambert* l_mat_surface{};
      const auto l_name = get_node_name(details::shading_engine_to_mat(l_mats.front()));
      if (extra_data_.material_map_->count(l_name) == 1) {
        l_mat_surface = extra_data_.material_map_->at(l_name);
      } else {
        l_mat_surface = FbxSurfaceLambert::Create(node->GetScene(), l_name.c_str());
        extra_data_.material_map_->emplace(l_name, l_mat_surface);
      }
      node->AddMaterial(l_mat_surface);
    } else {
      l_mat_layer->SetMappingMode(fbxsdk::FbxLayerElement::eByPolygon);
      l_mat_layer->SetReferenceMode(fbxsdk::FbxLayerElement::eIndexToDirect);
      MFnSet l_fn_set{};
      l_mat_ids.resize(l_fn_mesh.numPolygons());
      MStatus l_status{};
      for (auto l_mat : l_mats) {
        FbxSurfaceLambert* l_mat_surface{};
        const auto l_name = get_node_name(details::shading_engine_to_mat(l_mat));
        if (extra_data_.material_map_->count(l_name) == 1) {
          l_mat_surface = extra_data_.material_map_->at(l_name);
        } else {
          l_mat_surface = FbxSurfaceLambert::Create(node->GetScene(), l_name.c_str());
          extra_data_.material_map_->emplace(l_name, l_mat_surface);
        }

        auto l_mat_index = node->AddMaterial(l_mat_surface);
        maya_chick(l_fn_set.setObject(l_mat));

        MSelectionList l_list{};
        maya_chick(l_fn_set.getMembers(l_list, false));
        MDagPath l_path{};
        MObject l_comp{};
        for (MItSelectionList l_it_geo{l_list}; !l_it_geo.isDone(); l_it_geo.next()) {
          maya_chick(l_it_geo.getDagPath(l_path, l_comp));
          if (l_path == l_mesh && !l_comp.isNull()) {
            MFnSingleIndexedComponent const l_fn_comp{l_comp, &l_status};
            maya_chick(l_status);
            MIntArray l_indices{};
            maya_chick(l_fn_comp.getElements(l_indices));
            for (int l_indice : l_indices) {
              l_mat_ids[l_indice] = l_mat_index;
            }
          }
        }
      }
      l_mat_layer->GetIndexArray().SetCount(l_mat_ids.size());
      for (auto i = 0; i < l_mat_ids.size(); ++i) {
        l_mat_layer->GetIndexArray().SetAt(i, l_mat_ids[i]);
      }
    }

    // 三角形
    MIntArray l_vert_list{};
    for (auto i = 0; i < l_fn_mesh.numPolygons(); ++i) {
      mesh->BeginPolygon(l_mat_ids.empty() ? -1 : l_mat_ids[i]);
      maya_chick(l_fn_mesh.getPolygonVertices(i, l_vert_list));
      for (auto j = 0; j < l_vert_list.length(); ++j) {
        mesh->AddPolygon(l_vert_list[j]);
      }
      mesh->EndPolygon();
    }
    mesh->BuildMeshEdgeArray();
  }

  [&]() {
    // get uv set names
    MStringArray l_uv_set_names{};
    maya_chick(l_fn_mesh.getUVSetNames(l_uv_set_names));
    for (auto i = 0; i < l_uv_set_names.length(); ++i) {
      log_info(fmt::format("uv set name: {}", l_uv_set_names[i]));
      auto* l_layer = mesh->CreateElementUV(l_uv_set_names[i].asChar());
      l_layer->SetMappingMode(fbxsdk::FbxLayerElement::eByPolygonVertex);
      l_layer->SetReferenceMode(fbxsdk::FbxLayerElement::eIndexToDirect);
      // for maya uv
      MFloatArray l_u{};
      MFloatArray l_v{};
      l_fn_mesh.getUVs(l_u, l_v, &l_uv_set_names[i]);
      for (auto j = 0; j < l_u.length(); ++j) {
        l_layer->GetDirectArray().Add(FbxVector2{l_u[j], l_v[j]});
      }

      auto l_face_count = l_fn_mesh.numPolygons();
      for (auto k = 0; k < l_face_count; ++k) {
        auto l_poly_len = l_fn_mesh.polygonVertexCount(k);
        for (std::int32_t j = 0; j < l_poly_len; ++j) {
          std::int32_t l_uv_id{};
          l_fn_mesh.getPolygonUVid(k, j, l_uv_id, &l_uv_set_names[i]);  // warning: 这个我们忽略返回值, 不去测试错误
          l_layer->GetIndexArray().Add(l_uv_id);
        }
      }
    }
  }();

  // normals
  {
    auto l_layer = mesh->CreateElementNormal();
    l_layer->SetMappingMode(fbxsdk::FbxLayerElement::eByPolygonVertex);
    l_layer->SetReferenceMode(fbxsdk::FbxLayerElement::eDirect);

    for (auto i = 0; i < l_fn_mesh.numPolygons(); ++i) {
      MIntArray l_vert_list{};
      maya_chick(l_fn_mesh.getPolygonVertices(i, l_vert_list));
      for (auto j = 0; j < l_vert_list.length(); ++j) {
        MVector l_normal{};
        maya_chick(l_fn_mesh.getFaceVertexNormal(i, l_vert_list[j], l_normal, MSpace::kObject));
        l_layer->GetDirectArray().Add(FbxVector4{l_normal.x, l_normal.y, l_normal.z});
      }
    }
  }
  // smoothing
  {
    auto l_layer = mesh->CreateElementSmoothing();
    l_layer->SetMappingMode(fbxsdk::FbxLayerElement::eByEdge);
    l_layer->SetReferenceMode(fbxsdk::FbxLayerElement::eDirect);

    for (auto i = 0; i < l_fn_mesh.numEdges(); ++i) {
      l_layer->GetDirectArray().Add(l_fn_mesh.isEdgeSmooth(i));
    }
  }
}

void fbx_node_mesh::build_skin() {
  if (mesh == nullptr) {
    log_error(fmt::format(" {} is not mesh", dag_path));
    return;
  }
  auto l_skin_obj = get_skin_custer();
  if (l_skin_obj.isNull()) {
    log_error(fmt::format(" {} is not skin", dag_path));
    return;
  }
  auto* l_sk = FbxSkin::Create(node->GetScene(), get_node_name(l_skin_obj).c_str());
  mesh->AddDeformer(l_sk);

  auto l_joint_list = find_joint(l_skin_obj);

  std::vector<MPlug> l_skin_world_matrix_plug_list{};
  {
    MStatus l_status{};
    auto l_skin_world_matrix_plug_list_plug = get_plug(l_skin_obj, "matrix");
    for (auto i = 0; i < l_skin_world_matrix_plug_list_plug.numElements(); ++i) {
      if (l_skin_world_matrix_plug_list_plug[i].isConnected()) {
        l_skin_world_matrix_plug_list.emplace_back(
            l_skin_world_matrix_plug_list_plug.elementByPhysicalIndex(i).source(&l_status)
        );
        maya_chick(l_status);
      }
    }
  }

  MFnSkinCluster l_skin_cluster{l_skin_obj};
  auto l_skinning_method = get_plug(l_skin_obj, "skinningMethod").asInt();
  l_sk->SetSkinningType(static_cast<FbxSkin::EType>(l_skinning_method + 1));

  std::map<MDagPath, fbx_node_ptr, details::cmp_dag> l_dag_tree_map{};
  for (auto l_it = extra_data_.tree_->begin(); l_it != extra_data_.tree_->end(); ++l_it) {
    if (ranges::find_if(l_joint_list, boost::lambda2::_1 == (*l_it)->dag_path) != std::end(l_joint_list)) {
      l_dag_tree_map.emplace((*l_it)->dag_path, (*l_it));
    }
  }
  std::map<MDagPath, FbxCluster*, details::cmp_dag> l_dag_fbx_map{};

  for (auto&& i : l_joint_list) {
    auto l_joint    = l_dag_tree_map[i];
    auto* l_cluster = FbxCluster::Create(node->GetScene(), "");
    l_cluster->SetLink(l_joint->node);
    l_cluster->SetLinkMode(FbxCluster::eNormalize);
    l_dag_fbx_map.emplace(i, l_cluster);
  }

  MStatus l_status{};
  MDagPath l_skin_mesh_path{};
  for (auto i = 0; i < l_skin_cluster.numOutputConnections(); ++i) {
    auto l_index = l_skin_cluster.indexForOutputConnection(i, &l_status);
    maya_chick(l_status);
    maya_chick(l_skin_cluster.getPathAtIndex(l_index, l_skin_mesh_path));

    MItGeometry l_it_geo{l_skin_mesh_path};
    log_info(fmt::format("写出皮肤簇 {} 顶点数 {}", l_skin_mesh_path, l_it_geo.count()));
    for (; !l_it_geo.isDone(); l_it_geo.next()) {
      auto l_com = l_it_geo.currentItem(&l_status);
      maya_chick(l_status);
      std::uint32_t l_influence_count{};
      MDoubleArray l_influence_weights{};
      maya_chick(l_skin_cluster.getWeights(l_skin_mesh_path, l_com, l_influence_weights, l_influence_count));
      // 写出权重
      for (auto j = 0; j < l_influence_count; ++j) {
        if (l_influence_weights[j] == 0) continue;
        auto l_cluster = l_dag_fbx_map[l_joint_list[j]];
        if (l_influence_weights[j] > 0) l_cluster->AddControlPointIndex(l_it_geo.index(), l_influence_weights[j]);
      }
    }
    break;
  }

  for (auto&& i : l_joint_list) {
    auto l_joint    = l_dag_tree_map[i];
    auto* l_cluster = l_dag_fbx_map[i];
    l_cluster->SetTransformMatrix(node->EvaluateGlobalTransform());
    fbxsdk::FbxAMatrix l_fbx_matrix{};
    if (!extra_data_.bind_post->contains(l_joint->dag_path)) {
      extra_data_.logger_->log(
          log_loc(), level::err,
          "本次导出文件可能出错, 最好寻找绑定, 出错的骨骼, {} 骨骼的bindpose上没有记录值, 使用skin上的值进行处理 ",
          l_joint->dag_path
      );

      auto l_node              = l_joint->dag_path.node();

      auto l_world_matrix_plug = get_plug(l_node, "worldMatrix");
      auto l_index             = ranges::distance(
          std::begin(l_skin_world_matrix_plug_list),
          ranges::find_if(l_skin_world_matrix_plug_list, boost::lambda2::_1 == l_world_matrix_plug)
      );
      if (l_index == l_skin_world_matrix_plug_list.size()) {
        log_error(fmt::format("can not find world matrix plug: {}", get_node_name(l_node)));
        throw_exception(doodle_error{fmt::format("没有寻找到绑定矩阵, 请寻求绑定解决: {}", get_node_name(l_node))});
      }

      auto l_post_plug = get_plug(l_skin_obj, "bindPreMatrix")[l_index];
      MObject l_post_handle{};
      maya_chick(l_post_plug.getValue(l_post_handle));
      const MFnMatrixData l_data{l_post_handle};
      auto l_world_matrix = l_data.matrix(&l_status).inverse();
      maya_chick(l_status);

      for (auto i = 0; i < 4; ++i)
        for (auto j = 0; j < 4; ++j) l_fbx_matrix.mData[i][j] = l_world_matrix[i][j];
    } else {
      auto l_matrix = extra_data_.bind_post->at(l_joint->dag_path).world_matrix.asMatrix();
      for (auto i = 0; i < 4; ++i)
        for (auto j = 0; j < 4; ++j) l_fbx_matrix.mData[i][j] = l_matrix[i][j];
    }
    l_cluster->SetTransformLinkMatrix(l_fbx_matrix);
    if (!l_sk->AddCluster(l_cluster)) {
      log_error(fmt::format("add cluster error: {}", node->GetName()));
    }
  }

  {  // build post
    auto* l_post = FbxPose::Create(node->GetScene(), fmt::format("{}_post", get_node_name(l_skin_obj)).c_str());
    l_post->SetIsBindPose(true);
    std::vector<fbx_node_ptr> post_add{};

    std::function<bool(fbx_tree_t::iterator)> l_iter{};
    l_iter = [&](fbx_tree_t::iterator in_parent) -> bool {
      bool l_r{};
      for (auto l_it = in_parent.begin(); l_it != in_parent.end(); ++l_it) {
        auto l_sub_has = l_iter(l_it);
        if (ranges::find_if(l_joint_list, boost::lambda2::_1 == (*l_it)->dag_path) != std::end(l_joint_list) ||
            (*l_it)->dag_path == dag_path || l_sub_has) {
          post_add.emplace_back(*l_it);
          l_r |= true;
        }
      }
      return l_r;
    };

    l_iter(extra_data_.tree_->begin());

    for (auto&& i : post_add) {
      l_post->Add(i->node, i->node->EvaluateGlobalTransform());
    }
    node->GetScene()->AddPose(l_post);
  }
  mesh->AddDeformer(l_sk);
}

void fbx_node_mesh::build_blend_shape() {
  auto l_bls = find_blend_shape();
  if (l_bls.empty()) return;

  MFnMesh l_mfn_mesh{};
  {
    auto l_mesh = dag_path;
    maya_chick(l_mesh.extendToShape());
    maya_chick(l_mfn_mesh.setObject(l_mesh));
  }

  MFnBlendShapeDeformer l_blend_shape{};
  auto l_fbx_bl = FbxBlendShape::Create(node->GetScene(), fmt::format("{}", get_node_name(l_bls[0])).c_str());
  mesh->AddDeformer(l_fbx_bl);
  FbxProperty::Create(l_fbx_bl->RootProperty, FbxStringDT, "RootGroup");
  log_info(fmt::format("{} build blend shape {}", dag_path, get_node_name(l_bls[0])));

  MStatus l_status{};
  auto l_skin_obj = get_skin_custer();
  for (auto&& i : l_bls) {
    maya_chick(l_blend_shape.setObject(i));

    auto l_input_target_plug_array = get_plug(i, "inputTarget");
    auto l_output_geometry_array   = get_plug(i, "outputGeometry");
    MPlug l_input_target_plug{};

    // 寻找正确的变形几何插头
    for (auto j = 0; j < l_input_target_plug_array.numElements(); ++j) {
      l_input_target_plug = l_input_target_plug_array.elementByPhysicalIndex(j, &l_status);
      if (l_status != MStatus::kSuccess) {
        log_error(fmt::format("blend shape {} inputTarget error", get_node_name(dag_path)));
        continue;
      }
      auto l_output_geometry = l_output_geometry_array.elementByPhysicalIndex(j, &l_status);
      if (l_status != MStatus::kSuccess) {
        log_error(fmt::format("blend shape {} outputGeometry error", get_node_name(dag_path)));
        continue;
      }
      bool l_is_skin{};
      for (MItDependencyGraph l_it_graph{
               l_output_geometry, MFn::kSkinClusterFilter, MItDependencyGraph::kDownstream,
               MItDependencyGraph::kDepthFirst, MItDependencyGraph::kNodeLevel, &l_status
           };
           !l_it_graph.isDone(); l_it_graph.next()) {
        maya_chick(l_status);
        if (l_it_graph.thisNode() == l_skin_obj) {
          l_is_skin = true;
          log_info(fmt::format("find skin {}", get_node_name(l_skin_obj)));
          break;
        }
      }
      if (l_is_skin) break;
    }

    if (l_input_target_plug.isNull()) {
      log_error(fmt::format("blend shape {} inputTarget is null", get_node_name(dag_path)));
      continue;
    }

    auto l_input_target_group_array = l_input_target_plug.child(0, &l_status);
    maya_chick(l_status);
    auto l_shape_count = l_input_target_group_array.evaluateNumElements(&l_status);
    maya_chick(l_status);
    for (auto j = 0; j < l_shape_count; ++j) {
      auto l_input_target_group = l_input_target_group_array.elementByPhysicalIndex(j, &l_status);
      maya_chick(l_status);
      auto l_input_target_item = l_input_target_group.child(0).elementByPhysicalIndex(0, &l_status);
      maya_chick(l_status);

      auto l_input_point_target = l_input_target_item.child(3, &l_status);
      maya_chick(l_status);
      auto l_input_components_target = l_input_target_item.child(4, &l_status);
      maya_chick(l_status);
      auto l_input_point_target_data_handle = l_input_point_target.asMDataHandle(&l_status);
      maya_chick(l_status);
      auto l_input_components_target_data_handle = l_input_components_target.asMDataHandle(&l_status);
      maya_chick(l_status);

      if (l_input_point_target_data_handle.type() != MFnData::Type::kPointArray) {
        log_info(fmt::format("blend shape {} point data type error", get_node_name(dag_path)));
        continue;
      }
      if (l_input_components_target_data_handle.type() != MFnData::Type::kComponentList) {
        log_info(fmt::format("blend shape {} component data type error", get_node_name(dag_path)));
        continue;
      }
      if (l_input_point_target_data_handle.data().isNull()) {
        log_info(fmt::format("blend shape {} point data is null", get_node_name(dag_path)));
        continue;
      }
      if (l_input_components_target_data_handle.data().isNull()) {
        log_info(fmt::format("blend shape {} component data is null", get_node_name(dag_path)));
        continue;
      }
      MFnPointArrayData l_point_data{l_input_point_target_data_handle.data(), &l_status};
      maya_chick(l_status);
      if (l_point_data.length() == 0) {
        continue;
      }
      {
        // 检查全部接近0, 全部接近0就直接不导出
        bool l_is_zero{true};
        for (auto k = 0; k < l_point_data.length(); ++k) {
          if (std::abs(l_point_data[k].x) < 0.0001 && std::abs(l_point_data[k].y) < 0.0001 &&
              std::abs(l_point_data[k].z) < 0.0001) {
            continue;
          } else {
            l_is_zero = false;
            break;
          }
        }
        if (l_is_zero) continue;
      }

      MFnComponentListData l_component_data{l_input_components_target_data_handle.data(), &l_status};
      maya_chick(l_status);

      std::vector<std::int32_t> l_point_index_main{};
      for (auto k = 0; k < l_component_data.length(); ++k) {
        MFnSingleIndexedComponent l_component{l_component_data[k], &l_status};
        maya_chick(l_status);
        MIntArray l_point_index{};
        maya_chick(l_component.getElements(l_point_index));
        for (auto l_index : l_point_index) {
          l_point_index_main.emplace_back(l_index);
        }
      }

      if (l_point_data.length() != l_point_index_main.size()) {
        log_error(fmt::format(
            "blend shape {} point data length {} != point index length {}", get_node_name(i), l_point_data.length(),
            l_point_index_main.size()
        ));
        continue;
      }

      auto l_bl_weight_plug = get_plug(i, "weight").elementByPhysicalIndex(j, &l_status);
      FbxProperty::Create(
          l_fbx_bl->RootProperty, FbxStringDT,
          fmt::format("RootGroup|{}", l_bl_weight_plug.partialName(false, false, false, true, false, true)).c_str()
      );
      auto l_fbx_bl_channel = FbxBlendShapeChannel::Create(
          node->GetScene(),
          fmt::format("{}", l_bl_weight_plug.partialName(true, false, false, true, false, true)).c_str()
      );
      l_fbx_bl->AddBlendShapeChannel(l_fbx_bl_channel);
      auto l_fbx_deform = FbxShape::Create(
          node->GetScene(),
          fmt::format("{}", l_bl_weight_plug.partialName(false, false, false, true, false, true)).c_str()
      );
      l_fbx_bl_channel->AddTargetShape(l_fbx_deform);
      blend_shape_channel_.emplace_back(l_bl_weight_plug, l_fbx_bl_channel);

      l_fbx_deform->InitControlPoints(mesh->GetControlPointsCount());
      auto* l_fbx_points = l_fbx_deform->GetControlPoints();

      // 顶点
      std::copy(mesh->GetControlPoints(), mesh->GetControlPoints() + mesh->GetControlPointsCount(), l_fbx_points);
      auto l_max_count = mesh->GetControlPointsCount();
      for (auto k = 0; k < l_point_index_main.size(); ++k) {
        l_fbx_points[l_point_index_main[k]] += FbxVector4{
            l_point_data[k].x,
            l_point_data[k].y,
            l_point_data[k].z,
        };
      }
    }
  }
}

MObject fbx_node_mesh::get_skin_custer() const {
  if (!dag_path.hasFn(MFn::kMesh)) return {};

  MStatus l_s{};
  MObject l_skin_cluster{};
  auto l_path = dag_path;
  maya_chick(l_path.extendToShape());
  /// \brief 获得组件点上下文
  auto l_shape = l_path.node(&l_s);
  maya_chick(l_s);

  /// 寻找高模的皮肤簇
  for (MItDependencyGraph i{l_shape, MFn::kSkinClusterFilter, MItDependencyGraph::Direction::kUpstream}; !i.isDone();
       i.next()) {
    l_skin_cluster = i.currentItem(&l_s);
    maya_chick(l_s);
  }
  return l_skin_cluster;
}

std::vector<MDagPath> fbx_node_mesh::find_joint(const MObject& in_msk) const {
  if (in_msk.isNull()) return {};
  MFnSkinCluster l_skin_cluster{in_msk};
  MDagPathArray l_joint_array{};
  MStatus l_status{};
  auto l_joint_count = l_skin_cluster.influenceObjects(l_joint_array, &l_status);
  maya_chick(l_status);

  std::vector<MDagPath> l_joint_vector{};
  for (auto i = 0; i < l_joint_count; ++i) {
    l_joint_vector.emplace_back(l_joint_array[i]);
  }
  return l_joint_vector;
}

std::vector<MObject> fbx_node_mesh::find_blend_shape() const {
  if (!dag_path.hasFn(MFn::kMesh)) return {};

  MStatus l_s{};
  std::vector<MObject> l_blend_shapes{};
  auto l_path = dag_path;
  maya_chick(l_path.extendToShape());
  /// \brief 获得组件点上下文
  auto l_shape = l_path.node(&l_s);
  maya_chick(l_s);

  for (MItDependencyGraph i{l_shape, MFn::kBlendShape, MItDependencyGraph::Direction::kUpstream}; !i.isDone();
       i.next()) {
    l_blend_shapes.emplace_back(i.currentItem(&l_s));
    maya_chick(l_s);
  }
  return l_blend_shapes;
}

void fbx_node_mesh::build_animation(const MTime& in_time) {
  if (!dag_path.isValid()) return;

  auto* l_layer = mesh->GetScene()->GetCurrentAnimationStack()->GetMember<FbxAnimLayer>();
  FbxTime l_fbx_time{};
  l_fbx_time.SetFrame(in_time.value(), maya_to_fbx_time(in_time.unit()));

  MStatus l_status{};
  for (auto&& [l_bl_weight_plug, l_blend_shape_channel_] : blend_shape_channel_) {
    auto* l_anim_curve = l_blend_shape_channel_->DeformPercent.GetCurve(l_layer, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_bl_weight_plug.asDouble() * 100);
    l_anim_curve->KeyModifyEnd();
  }
}

MObject fbx_node_mesh::get_bind_post() const {
  auto l_skin = get_skin_custer();
  MStatus l_s{};
  /// 寻找高模的皮肤簇
  for (MItDependencyGraph i{l_skin, MFn::kDagPose, MItDependencyGraph::Direction::kUpstream}; !i.isDone(); i.next()) {
    auto l_obj = i.currentItem(&l_s);
    maya_chick(l_s);
    return l_obj;
  }
  auto l_joint_list = find_joint(get_skin_custer());
  for (const auto& l_j : l_joint_list) {
    auto l_shape = l_j.node(&l_s);
    maya_chick(l_s);

    /// 寻找高模的皮肤簇
    for (MItDependencyGraph i{l_shape, MFn::kDagPose, MItDependencyGraph::Direction::kDownstream}; !i.isDone();
         i.next()) {
      auto l_obj = i.currentItem(&l_s);
      maya_chick(l_s);
      return l_obj;
    }
  }

  return MObject::kNullObj;
}

void fbx_node_joint::build_data() {
  auto* l_sk_attr = FbxSkeleton::Create(node->GetScene(), "skeleton");
  l_sk_attr->SetSkeletonType(FbxSkeleton::eLimbNode);
  MStatus l_status{};
  auto l_is_ = get_plug(dag_path.node(), "segmentScaleCompensate").asBool(&l_status);
  maya_chick(l_status);
  auto l_size = get_plug(dag_path.node(), "radius").asDouble(&l_status);
  maya_chick(l_status);

  l_sk_attr->Size.Set(l_size);
  node->RotationActive.Set(true);
  auto l_vector_x = get_plug(dag_path.node(), "jointOrientX").asMAngle(&l_status);
  maya_chick(l_status);
  auto l_vector_y = get_plug(dag_path.node(), "jointOrientY").asMAngle(&l_status);
  maya_chick(l_status);
  auto l_vector_z = get_plug(dag_path.node(), "jointOrientZ").asMAngle(&l_status);
  maya_chick(l_status);
  node->PreRotation.Set(FbxVector4{l_vector_x.asDegrees(), l_vector_y.asDegrees(), l_vector_z.asDegrees()});
  //      node->SetPreRotation(FbxNode::eSourcePivot, FbxVector4{l_vector_x, l_vector_y, l_vector_z});
  node->UpdatePivotsAndLimitsFromProperties();
  node->SetNodeAttribute(l_sk_attr);
  build_node_transform(dag_path);

  auto l_parent_path = dag_path;
  l_parent_path.pop();
  if (l_parent_path.hasFn(MFn::kTransform) && !l_parent_path.hasFn(MFn::kJoint)) {
    l_is_ = false;
  }
  node->InheritType.Set(l_is_ ? FbxTransform::eInheritRrs : FbxTransform::eInheritRSrs);
}

}  // namespace fbx_write_ns

fbx_write::fbx_write() {
  manager_ = std::shared_ptr<FbxManager>{FbxManager::Create(), [](FbxManager* in_ptr) { in_ptr->Destroy(); }};
  scene_   = FbxScene::Create(manager_.get(), "doodle_to_ue_fbx");
  manager_->SetIOSettings(FbxIOSettings::Create(manager_.get(), IOSROOT));
  auto l_doc_info      = FbxDocumentInfo::Create(manager_.get(), "DocInfo");
  l_doc_info->mTitle   = "doodle fbx";
  l_doc_info->mSubject = "doodle fbx";
  l_doc_info->mAuthor  = "doodle";
  l_doc_info->Original_ApplicationVendor.Set("doodle");
  l_doc_info->Original_ApplicationName.Set("doodle");
  l_doc_info->Original_ApplicationVersion.Set("1.0.0");

  l_doc_info->LastSaved_ApplicationVendor.Set("doodle");
  l_doc_info->LastSaved_ApplicationName.Set("doodle");
  l_doc_info->LastSaved_ApplicationVersion.Set("1.0.0");

  scene_->SetSceneInfo(l_doc_info);
  scene_->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
  scene_->GetGlobalSettings().SetTimeMode(fbx_write_ns::fbx_node::maya_to_fbx_time(MTime::uiUnit()));

  auto* anim_stack = FbxAnimStack::Create(scene_, "anim_stack");
  auto* anim_layer = FbxAnimLayer::Create(scene_, "anim_layer");
  anim_stack->AddMember(anim_layer);
  scene_->SetCurrentAnimationStack(anim_stack);
}

void fbx_write::write(
    const std::vector<MDagPath>& in_vector, const std::vector<MDagPath>& in_sim_vector, const MTime& in_begin,
    const MTime& in_end
) {
  if (!logger_)
    if (!g_ctx().contains<fbx_logger>())
      logger_ =
          g_ctx()
              .emplace<fbx_logger>(g_logger_ctrl().make_log_file(path_.parent_path() / "fbx_log.txt", "fbx_logger"))
              .logger_;
    else
      logger_ = g_ctx().get<fbx_logger>().logger_;

  log_info(logger_, fmt::format("开始导出文件 {}", path_.generic_string()));

  auto* anim_stack = scene_->GetCurrentAnimationStack();
  FbxTime l_fbx_begin{};
  l_fbx_begin.SetFrame(in_begin.value(), fbx_write_ns::fbx_node::maya_to_fbx_time(in_begin.unit()));
  anim_stack->LocalStart = l_fbx_begin;
  FbxTime l_fbx_end{};
  l_fbx_end.SetFrame(in_end.value(), fbx_write_ns::fbx_node::maya_to_fbx_time(in_end.unit()));
  anim_stack->LocalStop = l_fbx_end;

  anim_time_            = {in_begin, in_end};

  MAnimControl::setCurrentTime(in_begin);

  std::vector<sequence_to_blend_shape> l_sequence_to_blend_shape{};
  try {
    init();
    build_tree(in_vector, in_sim_vector);
    build_data();

    // 初始化解算分解器
    std::int64_t l_size = in_begin.value() - in_end.value() + 1;
    for (auto&& i : in_sim_vector) {
      l_sequence_to_blend_shape.emplace_back(i, l_size);
    }

    log_info(logger_, "开始导出动画");

    if (export_anim_) {
      for (auto l_time = in_begin; l_time <= in_end; ++l_time) {
        MAnimControl::setCurrentTime(l_time);
        build_animation(l_time);
        for (auto&& i : l_sequence_to_blend_shape) {
          i.add_sample(l_time.value() - in_begin.value());
        }
      }
    }
    for (auto&& i : l_sequence_to_blend_shape) {
      i.compute();
    }
    for (auto&& i : l_sequence_to_blend_shape) {
      i.write_fbx(*this);
    }

  } catch (const std::exception& in_error) {
    auto l_str = boost::diagnostic_information(in_error);
    MGlobal::displayError(conv::to_ms(l_str));
    log_error(logger_, fmt::format("导出文件 {} 错误 {}", path_, l_str));
    return;
  }

  logger_->flush();
}

std::vector<MDagPath> fbx_write::select_to_vector(const MSelectionList& in_vector) {
  std::vector<MDagPath> l_objs{};
  MDagPath l_path{};
  for (MItSelectionList l_it{in_vector}; !l_it.isDone(); l_it.next()) {
    maya_chick(l_it.getDagPath(l_path));
    l_objs.emplace_back(l_path);
  }
  return l_objs;
}

// void fbx_write::write(MDagPath in_cam_path, const MTime& in_begin, const MTime& in_end) {
//   path_            = in_path;
//   auto* anim_stack = scene_->GetCurrentAnimationStack();
//   FbxTime l_fbx_begin{};
//   l_fbx_begin.SetFrame(in_begin.value(), fbx_write_ns::fbx_node::maya_to_fbx_time(in_begin.unit()));
//   anim_stack->LocalStart = l_fbx_begin;
//   FbxTime l_fbx_end{};
//   l_fbx_end.SetFrame(in_end.value() + 1, fbx_write_ns::fbx_node::maya_to_fbx_time(in_end.unit()));
//   anim_stack->LocalStop = l_fbx_end;

//   MAnimControl::setCurrentTime(find_begin_anim_time());

//   init();
//   //  build_tree(in_vector);
//   return;
//   try {
//     build_data();
//   } catch (const maya_error& in_error) {
//     auto l_str = boost::diagnostic_information(in_error);
//     MGlobal::displayError(conv::to_ms(l_str));
//     log_error(logger_, fmt::format("导出文件 {} 错误 {}", path_, l_str));
//     return;
//   } catch (const doodle_error& in_error) {
//     auto l_str = boost::diagnostic_information(in_error);
//     MGlobal::displayError(conv::to_ms(l_str));
//     log_error(logger_, fmt::format("导出文件 {} 错误 {}", path_, l_str));
//     return;
//   }

//   if (export_anim_) {
//     for (auto l_time = in_begin; l_time <= in_end; ++l_time) {
//       MAnimControl::setCurrentTime(l_time);
//       build_animation(l_time);
//     }
//   }
//   logger_->flush();
// }

void fbx_write::init() { tree_ = {std::make_shared<fbx_node_transform_t>(MDagPath{}, scene_->GetRootNode())}; }
void fbx_write::build_tree(const std::vector<MDagPath>& in_vector, const std::vector<MDagPath>& in_sim_vector) {
  auto l_fun = [this](const std::vector<MDagPath>& in_vector, bool in_is_sim) {
    for (auto l_path : in_vector) {
      auto l_begin = tree_.begin();
      for (std::int32_t i = l_path.length() - 1; i >= 0; --i) {
        MDagPath l_sub_path{l_path};
        l_sub_path.pop(i);

        if (auto l_tree_it = ranges::find_if(
                std::begin(l_begin), std::end(l_begin),
                [&](const fbx_node_ptr& in_value) { return in_value->dag_path == l_sub_path; }
            );
            l_tree_it != std::end(l_begin)) {
          l_begin = l_tree_it;
        } else {
          auto l_parent_node = (*l_begin)->node;

          if (l_sub_path.hasFn(MFn::kMesh)) {
            auto l_mesh = std::make_shared<fbx_node_mesh_t>(
                l_sub_path, FbxNode::Create(scene_, get_node_name(l_sub_path).c_str())
            );
            l_mesh->is_sim = in_is_sim;
            joints_ |= ranges::action::push_back(l_mesh->find_joint(l_mesh->get_skin_custer()));

            l_begin = tree_.append_child(l_begin, l_mesh);
          } else if (l_sub_path.hasFn(MFn::kJoint)) {
            l_begin = tree_.append_child(
                l_begin, std::make_shared<fbx_node_joint_t>(
                             l_sub_path, FbxNode::Create(scene_, get_node_name(l_sub_path).c_str())
                         )
            );
          } else {
            l_begin = tree_.append_child(
                l_begin, std::make_shared<fbx_node_transform_t>(
                             l_sub_path, FbxNode::Create(scene_, get_node_name(l_sub_path).c_str())
                         )
            );
          }
          node_map_.emplace(l_sub_path, *l_begin);
          l_parent_node->AddChild((*l_begin)->node);
        }
      }
    }
  };

  // 这个是绑定的网格
  l_fun(in_vector, false);

  // 这个是模拟的网格
  l_fun(in_sim_vector, true);

  for (auto&& i : joints_) {
    auto l_begin = tree_.begin();
    for (std::int32_t j = i.length() - 1; j >= 0; --j) {
      MDagPath l_sub_path{i};
      l_sub_path.pop(j);

      if (auto l_tree_it = ranges::find_if(
              std::begin(l_begin), std::end(l_begin),
              [&](const fbx_node_ptr& in_value) { return in_value->dag_path == l_sub_path; }
          );
          l_tree_it != std::end(l_begin)) {
        l_begin = l_tree_it;
      } else {
        auto l_parent_node = (*l_begin)->node;

        if (l_sub_path.hasFn(MFn::kJoint)) {
          l_begin = tree_.append_child(
              l_begin,
              std::make_shared<fbx_node_joint_t>(l_sub_path, FbxNode::Create(scene_, get_node_name(l_sub_path).c_str()))
          );
        } else {
          l_begin = tree_.append_child(
              l_begin, std::make_shared<fbx_node_transform_t>(
                           l_sub_path, FbxNode::Create(scene_, get_node_name(l_sub_path).c_str())
                       )
          );
        }
        l_parent_node->AddChild((*l_begin)->node);
        node_map_.emplace(l_sub_path, *l_begin);
      }
    }
  }

  std::function<bool(fbx_node_ptr&)> l_iter_fun{};

  l_iter_fun = [&](fbx_node_ptr& in_iterator) { return in_iterator->dag_path.hasFn(MFn::kJoint); };

  if (auto l_it = ranges::find_if(tree_.begin(), tree_.end(), l_iter_fun); l_it != tree_.end()) {
    MDagPath l_parent_path{};
    MItDag l_dag_it{};
    l_dag_it.reset((*l_it)->dag_path, MItDag::kDepthFirst);
    for (; !l_dag_it.isDone(); l_dag_it.next()) {
      MDagPath l_path{};
      maya_chick(l_dag_it.getPath(l_path));

      if (auto l_tree_it = ranges::find_if(
              std::begin(tree_), std::end(tree_),
              [&](fbx_node_ptr& in_value) -> bool { return in_value->dag_path == l_path; }
          );
          l_tree_it == std::end(tree_) && (l_path.hasFn(MFn::kTransform) && !l_path.hasFn(MFn::kConstraint))) {
        l_parent_path = l_path;
        l_parent_path.pop();
        auto l_tree_parent = ranges::find_if(std::begin(tree_), std::end(tree_), [&](fbx_node_ptr& in_value) -> bool {
          return in_value->dag_path == l_parent_path;
        });
        if (l_tree_parent == std::end(tree_)) {
          log_error(logger_, fmt::format("找不到父节点 {}, 此处可能是有错误的绑定", l_parent_path));
          continue;
        }
        auto l_parent_node = (*l_tree_parent)->node;

        fbx_tree_t ::iterator l_node_iter{};
        if (l_path.hasFn(MFn::kJoint)) {
          l_node_iter = tree_.append_child(
              l_tree_parent,
              std::make_shared<fbx_node_joint_t>(l_path, FbxNode::Create(scene_, get_node_name(l_path).c_str()))
          );
        } else {
          l_node_iter = tree_.append_child(
              l_tree_parent,
              std::make_shared<fbx_node_transform_t>(l_path, FbxNode::Create(scene_, get_node_name(l_path).c_str()))
          );
        }
        l_parent_node->AddChild((*l_node_iter)->node);
        node_map_.emplace(l_path, *l_node_iter);
      }
    }
  }
}
void fbx_write::build_data() {
  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_init{};
  l_iter_init = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      (*i)->extra_data_.tree_            = &tree_;
      (*i)->extra_data_.material_map_    = &material_map_;
      (*i)->extra_data_.bind_post        = &bind_post_;
      (*i)->extra_data_.bind_pose_array_ = &bind_pose_array_;
      (*i)->extra_data_.logger_          = logger_;
      l_iter_init(i);
    }
  };
  l_iter_init(tree_.begin());

  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_bind_post{};

  l_iter_bind_post = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      if ((*i)->dag_path.hasFn(MFn::kMesh))
        if (auto l_ptr = std::dynamic_pointer_cast<fbx_node_mesh_t>(*i); l_ptr) {
          l_ptr->build_bind_post();
        }
      l_iter_bind_post(i);
    }
    return false;
  };
  l_iter_bind_post(tree_.begin());

  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_fun_tran{};
  l_iter_fun_tran = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      if (!(*i)->dag_path.hasFn(MFn::kMesh)) (*i)->build_node();
      l_iter_fun_tran(i);
    }
  };

  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_fun_mesh{};
  l_iter_fun_mesh = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      (*i)->build_node();
      l_iter_fun_mesh(i);
    }
  };

  l_iter_fun_tran(tree_.begin());
  l_iter_fun_mesh(tree_.begin());
}
void fbx_write::build_animation(const MTime& in_time) {
  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_fun{};
  l_iter_fun = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      (*i)->build_animation(in_time);
      l_iter_fun(i);
    }
  };
  l_iter_fun(tree_.begin());
}

MTime fbx_write::find_begin_anim_time() {
  MTime l_begin_time{MAnimControl::minTime()};
  MFnAnimCurve l_anim_curve{};
  for (MItDependencyNodes it{MFn::Type::kAnimCurve}; !it.isDone(); it.next()) {
    auto l_obj = it.thisNode();
    if (l_obj.hasFn(MFn::Type::kAnimCurveTimeToAngular) || l_obj.hasFn(MFn::Type::kAnimCurveTimeToDistance) ||
        l_obj.hasFn(MFn::Type::kAnimCurveTimeToTime) || l_obj.hasFn(MFn::Type::kAnimCurveTimeToUnitless)) {
      maya_chick(l_anim_curve.setObject(it.thisNode()));
      auto l_key_count = l_anim_curve.numKeys();
      if (l_key_count == 0) continue;
      auto l_time = l_anim_curve.time(0);
      if (l_time < l_begin_time) l_begin_time = l_time;
    }
  }
  return l_begin_time;
}

void fbx_write::not_export_anim(bool in_value) { export_anim_ = !in_value; }
void fbx_write::ascii_fbx(bool in_value) { ascii_fbx_ = in_value; }
void fbx_write::set_path(const FSys::path& in_path) { path_ = in_path; }
void fbx_write::set_logger(const logger_ptr& in_logger) { logger_ = in_logger; }
std::pair<MTime, MTime> fbx_write::get_anim_time() const { return anim_time_; };

fbx_write_ns::fbx_node* fbx_write::find_node(const MDagPath& in_path) const {
  auto l_it = std::find_if(std::begin(tree_), std::end(tree_), [&](const fbx_node_ptr& in_value) -> bool {
    return in_value->dag_path == in_path;
  });
  if (l_it == std::end(tree_)) return nullptr;
  return l_it->get();
}

fbxsdk::FbxSurfaceLambert* fbx_write::find_material(const std::string& in_obj) const {
  auto l_it = material_map_.find(in_obj);
  if (l_it == std::end(material_map_)) {
    auto l_material = FbxSurfaceLambert::Create(scene_->GetFbxManager(), in_obj.c_str());
    material_map_.emplace(in_obj, l_material);
    return l_material;
  }
  return l_it->second;
}

void fbx_write::write_end() {
  if (!manager_) return;
  if (!scene_) return;
  if (path_.empty()) return;

  std::shared_ptr<FbxExporter> l_exporter{
      FbxExporter::Create(scene_->GetFbxManager(), ""), [](FbxExporter* in_exporter) { in_exporter->Destroy(); }
  };
  manager_->GetIOSettings()->SetBoolProp(EXP_FBX_MATERIAL, true);
  manager_->GetIOSettings()->SetBoolProp(EXP_FBX_TEXTURE, true);
  manager_->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED, true);
  manager_->GetIOSettings()->SetBoolProp(EXP_FBX_SHAPE, true);
  manager_->GetIOSettings()->SetBoolProp(EXP_FBX_GOBO, true);
  manager_->GetIOSettings()->SetBoolProp(EXP_FBX_ANIMATION, true);
  manager_->GetIOSettings()->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
  manager_->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, true);

  if (!l_exporter->Initialize(
          path_.generic_string().c_str(),
          ascii_fbx_ ? manager_->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)")
                     : manager_->GetIOPluginRegistry()->GetNativeWriterFormat(),
          scene_->GetFbxManager()->GetIOSettings()
      )) {
    MGlobal::displayError(
        conv::to_ms(fmt::format("fbx exporter Initialize error: {}", l_exporter->GetStatus().GetErrorString()))
    );
    return;
  }
  l_exporter->Export(scene_);
}

fbx_write::~fbx_write() { write_end(); }
}  // namespace doodle::maya_plug
