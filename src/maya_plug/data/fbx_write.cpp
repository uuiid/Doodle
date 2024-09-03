//
// Created by td_main on 2023/10/16.
//

#include "fbx_write.h"

#include <doodle_core/exception/exception.h>

#include <boost/lambda2.hpp>
#include <boost/math/special_functions/relative_difference.hpp>

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
#include <numbers>
#include <treehh/tree.hh>
namespace doodle::maya_plug {
namespace fbx_write_ns {

inline std::double_t radians_to_degrees(const std::double_t in_radians) {
  MAngle l_angle{};
  l_angle.setUnit(MAngle::kRadians);
  l_angle.setValue(in_radians);
  return l_angle.asDegrees();
}

inline std::double_t degrees_to_radians(const std::double_t in_degrees) {
  MAngle l_angle{};
  l_angle.setUnit(MAngle::kDegrees);
  l_angle.setValue(in_degrees);
  return l_angle.asRadians();
}

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
  camera_ = FbxCamera::Create(node->GetScene(), get_node_name(dag_path).c_str());
  node->SetNodeAttribute(camera_);

  MFnTransform const l_transform{dag_path};
  MStatus l_status{};
  {
    const auto l_pose_quaternion = l_transform.transformation(&l_status).rotationOrientation();
    maya_chick(l_status);
    // auto l_x_rot                 = l_pose_quaternion.asEulerRotation().x + std::numbers::pi / 2;  // 弧度表示
    // auto l_axi                   = MVector{0, std::sin(l_x_rot), std::cos(l_x_rot)};
    // default_logger_raw()->info("axi {}", l_axi);
    MQuaternion l_rot{std::numbers::pi / 2, {0, -1, 0}};
    auto l_tmp_rot = (l_pose_quaternion.inverse() * l_rot).asEulerRotation();

    node->Freeze.Set(true);
    node->PostRotation.Set(
        {radians_to_degrees(l_tmp_rot.x), radians_to_degrees(l_tmp_rot.y), radians_to_degrees(l_tmp_rot.z)}
    );
  }
  build_node_transform(dag_path);

  MFnCamera l_fn_cam{dag_path};
  camera_->ProjectionType.Set(l_fn_cam.isOrtho() ? FbxCamera::eOrthogonal : FbxCamera::ePerspective);
  std::int32_t l_width{}, l_height{};
  std::double_t l_horizontal_fov{}, l_vertical_fov{};
  l_fn_cam.getPortFieldOfView(l_width, l_height, l_horizontal_fov, l_vertical_fov);
  camera_->SetAspect(FbxCamera::EAspectRatioMode::eWindowSize, l_width, l_height);

  camera_->SetApertureWidth(l_fn_cam.horizontalFilmAperture());
  camera_->SetApertureHeight(l_fn_cam.verticalFilmAperture());
  camera_->SetApertureMode(FbxCamera::eFocalLength);

  if (boost::math::relative_difference(l_fn_cam.horizontalFilmAperture() / l_fn_cam.verticalFilmAperture(), 1.78) >
      0.005) {
    throw_error(::doodle::maya_enum::maya_error_t::camera_aspect_error);
  }

  camera_->FocalLength.Set(l_fn_cam.focalLength());
  camera_->FocusDistance.Set(l_fn_cam.focusDistance());
  if (!camera_->FocusDistance.GetFlag(FbxPropertyFlags::EFlags::eAnimatable))
    camera_->FocusDistance.ModifyFlag(FbxPropertyFlags::EFlags::eAnimatable, true);
  camera_->FilmAspectRatio.Set(l_fn_cam.aspectRatio());
  camera_->Position.Set(camera_->EvaluatePosition());
  {
    auto l_up = l_fn_cam.upDirection(MSpace::Space::kWorld);
    if (!camera_->UpVector.GetFlag(FbxPropertyFlags::EFlags::eNotSavable))
      camera_->UpVector.ModifyFlag(FbxPropertyFlags::EFlags::eNotSavable, false);
    camera_->UpVector.Set({l_up.x, l_up.y, l_up.z});
  }
  {
    auto l_p = l_fn_cam.centerOfInterestPoint(MSpace::kWorld);
    camera_->InterestPosition.Set({l_p.x, l_p.y, l_p.z});
  }

  // 相机的缩放强制设置为1
  node->LclScaling.Set({1.0, 1.0, 1.0});
}

void fbx_node_cam::build_animation(const MTime& in_time) {
  FbxTime l_fbx_time{};
  l_fbx_time.SetFrame(in_time.value(), maya_to_fbx_time(in_time.unit()));

  auto* l_layer = node->GetScene()->GetCurrentAnimationStack()->GetMember<FbxAnimLayer>();
  MStatus l_status{};
  MFnCamera l_fn_cam{dag_path};

  MFnTransform const l_transform{dag_path};
  // tran x
  {
    auto* l_anim_curve = node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_tran_x    = l_transform.getTranslation(MSpace::kWorld).x;
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_tran_x);
    l_anim_curve->KeyModifyEnd();
  }
  // tran y
  {
    auto* l_anim_curve = node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_tran_y    = l_transform.getTranslation(MSpace::kWorld).y;
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_tran_y);
    l_anim_curve->KeyModifyEnd();
  }
  // tran z
  {
    auto* l_anim_curve = node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_tran_z    = l_transform.getTranslation(MSpace::kWorld).z;
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_tran_z);
    l_anim_curve->KeyModifyEnd();
  }

  {
    MQuaternion l_rot{};

    l_transform.getRotation(l_rot, MSpace::kWorld);
    auto l_rot_tmp = l_rot.asEulerRotation();
    MAngle l_ang{};
    // rot x
    {
      auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
      l_anim_curve->KeyModifyBegin();
      auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
      maya_chick(l_status);
      l_ang.setUnit(MAngle::kRadians);
      l_ang.setValue(l_rot_tmp.x);
      l_anim_curve->KeySet(l_key_index, l_fbx_time, l_ang.asDegrees());
      l_anim_curve->KeyModifyEnd();
    }

    // rot y
    {
      auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
      l_anim_curve->KeyModifyBegin();
      auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
      maya_chick(l_status);
      l_ang.setUnit(MAngle::kRadians);
      l_ang.setValue(l_rot_tmp.y);
      l_anim_curve->KeySet(l_key_index, l_fbx_time, l_ang.asDegrees());
      l_anim_curve->KeyModifyEnd();
    }

    // rot z
    {
      auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
      l_anim_curve->KeyModifyBegin();
      auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
      maya_chick(l_status);
      l_ang.setUnit(MAngle::kRadians);
      l_ang.setValue(l_rot_tmp.z);
      l_anim_curve->KeySet(l_key_index, l_fbx_time, l_ang.asDegrees());
      l_anim_curve->KeyModifyEnd();
    }
  }

  {
    auto* l_cureve = camera_->FocalLength.GetCurve(l_layer, true);
    l_cureve->KeyModifyBegin();
    auto l_index = l_cureve->KeyAdd(l_fbx_time);
    l_cureve->KeySetValue(l_index, l_fn_cam.focalLength());
    l_cureve->KeyModifyEnd();
  }
  // camera_->FocusDistance.Set(l_fn_cam.focusDistance());
  {
    // auto* l_node = camera_->FocusDistance.CreateCurveNode(l_layer);
    auto* l_cureve = camera_->FocusDistance.GetCurve(l_layer, true);
    l_cureve->KeyModifyBegin();
    auto l_index = l_cureve->KeyAdd(l_fbx_time);
    l_cureve->KeySetValue(l_index, l_fn_cam.focusDistance());
    l_cureve->KeyModifyEnd();
  }
  // camera_->FilmAspectRatio.Set(l_fn_cam.aspectRatio());

  // camera_->Position.Set(camera_->EvaluatePosition());
  // {
  //   auto l_cureve = camera_->Position.GetCurve(l_layer, true);
  //   l_cureve->KeyModifyBegin();
  //   auto l_index = l_cureve->KeyAdd(l_fbx_time);
  //   l_cureve->KeySetValue(l_index, camera_->EvaluatePosition());
  //   l_cureve->KeyModifyEnd();
  // }
}

///

////
void fbx_node_transform::build_data() {
  build_node_transform(dag_path);
  auto l_attr_null = FbxNull::Create(node->GetScene(), get_node_name(dag_path).c_str());
  l_attr_null->Look.Set(FbxNull::eNone);
  node->SetNodeAttribute(l_attr_null);
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

  auto l_size_x = get_plug(dag_path.node(), "scaleX").asDouble();
  auto l_size_y = get_plug(dag_path.node(), "scaleY").asDouble();
  auto l_size_z = get_plug(dag_path.node(), "scaleZ").asDouble();
  if (boost::math::relative_difference(l_size_x, 0.0000001) && boost::math::relative_difference(l_size_y, 0.0000001) &&
      boost::math::relative_difference(l_size_z, 0.0000001)) {
    default_logger_raw()->error("{} 缩放为 0", dag_path);
    throw_error(doodle::maya_enum::maya_error_t::bone_scale_error, fmt::format(" {} 缩放为 0", dag_path));
  }
  // size x
  {
    auto* l_anim_curve = node->LclScaling.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_size_x);
    l_anim_curve->KeyModifyEnd();
  }
  // size y
  {
    auto* l_anim_curve = node->LclScaling.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_size_y);
    l_anim_curve->KeyModifyEnd();
  }
  // size z
  {
    auto* l_anim_curve = node->LclScaling.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_size_z);
    l_anim_curve->KeyModifyEnd();
  }
}

////

void fbx_node_mesh::build_data() {
  if (!dag_path.isValid()) return;
  fbx_node_transform::build_data();

  build_mesh();
  build_skin();
  build_blend_shape();
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
    // 材质
    std::vector<std::int32_t> l_mat_ids{};
    fbxsdk::FbxGeometryElementMaterial* l_mat_layer{};
    if (mesh->GetElementMaterialCount() == 0) {
      l_mat_layer = mesh->CreateElementMaterial();
    } else {
      l_mat_layer = mesh->GetElementMaterial(0);
    }

    auto l_mats = get_shading_engines(l_mesh);
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

  // uv
  std::vector<fbxsdk::FbxGeometryElementUV*> l_uvs{};
  for (auto i = 0; i < mesh->GetElementUVCount(); ++i) {
    l_uvs.push_back(mesh->GetElementUV(i));
  }
  // delete old uv
  for (auto i = 0; i < l_uvs.size(); ++i) {
    mesh->RemoveElementUV(l_uvs[i]);
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
    // 这里有fbx 的怪癖,  要一个layer, 将所有的normal写进去, 不可以直接创建 Normal layer
    FbxLayer* l_layer = mesh->GetLayer(0);
    if (!l_layer) {
      mesh->CreateLayer();
      l_layer = mesh->GetLayer(0);
    }
    FbxLayerElementNormal* l_layer_n = FbxLayerElementNormal::Create(mesh, "");
    l_layer_n->SetMappingMode(FbxLayerElement::eByPolygonVertex);
    l_layer_n->SetReferenceMode(FbxLayerElement::eDirect);
    default_logger_raw()->info("写出mesh normal {}", node->GetName());

    for (auto i = 0; i < l_fn_mesh.numPolygons(); ++i) {
      MIntArray l_vert_list{};
      maya_chick(l_fn_mesh.getPolygonVertices(i, l_vert_list));
      for (auto j = 0; j < l_vert_list.length(); ++j) {
        MVector l_normal{};
        maya_chick(l_fn_mesh.getFaceVertexNormal(i, l_vert_list[j], l_normal, MSpace::kObject));
        l_layer_n->GetDirectArray().Add(FbxVector4{l_normal.x, l_normal.y, l_normal.z});
      }
    }
    l_layer->SetNormals(l_layer_n);
  }
  // smoothing
  {
    fbxsdk::FbxGeometryElementSmoothing* l_layer;
    if (mesh->GetElementSmoothingCount() == 0) {
      l_layer = mesh->CreateElementSmoothing();
    } else {
      l_layer = mesh->GetElementSmoothing(0);
    }
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
  default_logger_raw()->info("使用皮肤簇 {}", get_node_name(l_skin_obj));
  if (l_skin_obj.isNull()) {
    log_error(fmt::format(" {} is not skin", dag_path));
    return;
  }
  auto* l_sk = FbxSkin::Create(node->GetScene(), get_node_name(l_skin_obj).c_str());
  mesh->AddDeformer(l_sk);

  auto l_joint_list = find_joint(l_skin_obj);

  MFnSkinCluster l_skin_cluster{l_skin_obj};
  auto l_skinning_method = get_plug(l_skin_obj, "skinningMethod").asInt();
  l_sk->SetSkinningType(static_cast<FbxSkin::EType>(l_skinning_method + 1));

  std::map<MDagPath, fbx_node_ptr, details::cmp_dag> l_dag_tree_map{};
  for (auto l_it = extra_data_.tree_->begin(); l_it != extra_data_.tree_->end(); ++l_it) {
    l_dag_tree_map.emplace((*l_it)->dag_path, (*l_it));
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
    log_info(fmt::format("写出网格体 {} 顶点数 {}", l_skin_mesh_path, l_it_geo.count()));
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

  // 使用 skin 节点上的 bindPreMatrix 属性迭代设置控制点的矩阵
  // 骨骼, 和皮肤簇中矩阵的对应关系
  std::map<MDagPath, MMatrix, details::cmp_dag> l_dag_matrix_map{};
  {
    auto l_matrix_plug          = get_plug(l_skin_obj, "matrix");
    auto l_bind_pre_matrix_plug = get_plug(l_skin_obj, "bindPreMatrix");
    for (auto i = 0; i < l_matrix_plug.numElements(); ++i) {
      if (l_matrix_plug[i].isConnected(&l_status) && l_status) {
        // 这里必须使用逻辑索以才会正确
        auto l_log_index = l_matrix_plug[i].logicalIndex(&l_status);
        maya_chick(l_status);
        auto l_bind_pre_matrix = l_bind_pre_matrix_plug.elementByLogicalIndex(l_log_index, &l_status);
        maya_chick(l_status);
        auto l_matrix_obj = l_bind_pre_matrix.asMObject(&l_status);
        maya_chick(l_status);
        MFnMatrixData l_matrix{};
        maya_chick(l_matrix.setObject(l_matrix_obj));
        auto l_soure = l_matrix_plug[i].source();
        // default_logger_raw()->info("{}",l_soure.info());
        auto l_m     = l_matrix.matrix(&l_status);
        maya_chick(l_status);
        l_dag_matrix_map.emplace(get_dag_path(l_soure.node()), l_m);
      }
    }
  }

  for (auto&& i : l_joint_list) {
    auto l_joint    = l_dag_tree_map[i];
    auto* l_cluster = l_dag_fbx_map[i];
    l_cluster->SetTransformMatrix(node->EvaluateGlobalTransform());
    FbxAMatrix l_fbx_matrix{};

    if (l_dag_matrix_map.contains(l_joint->dag_path)) {
      auto l_world_matrix = l_dag_matrix_map[l_joint->dag_path].inverse();
      for (auto i2 = 0; i2 < 4; ++i2)
        for (auto j = 0; j < 4; ++j) l_fbx_matrix.mData[i2][j] = l_world_matrix[i2][j];
    } else {
      l_fbx_matrix.SetIdentity();
      default_logger_raw()->warn("无法获取矩阵 {}", l_joint->dag_path);
    }
    l_cluster->SetTransformLinkMatrix(l_fbx_matrix);
    if (!l_sk->AddCluster(l_cluster)) {
      log_error(fmt::format("add cluster error: {}", node->GetName()));
    }
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
      auto l_bl_weight_plug = get_plug(i, "weight").elementByPhysicalIndex(j, &l_status);

      {
        // 去除特定名称的混变
        MString l_name = l_bl_weight_plug.partialName(false, false, false, true, false, true);
        constexpr static std::array l_name_list{
            std::string_view{"EyeLidLayer"},       std::string_view{"SquintLayer"},
            std::string_view{"EyeBrowLayer"},      std::string_view{"LipLayer"},
            std::string_view{"JawLayer"},          std::string_view{"zipperLips_RLayer"},
            std::string_view{"zipperLips_LLayer"}, std::string_view{"NoseLayer"},
            std::string_view{"SmilePullLayer"},    std::string_view{"SmileBulgeLayer"},
            std::string_view{"CheekRaiserLayer"},  std::string_view{"MouthNarrowLayer"},
            std::string_view{"CheekLayer"},        std::string_view{"RegionsLayer"},
            std::string_view{"UpMidLoLayer"},      std::string_view{"asFaceBS"},
        };
        if (std::any_of(std::begin(l_name_list), std::end(l_name_list), [&](const std::string_view& in_name) -> bool {
              return conv::to_s(l_name) == in_name;
            })) {
          default_logger_raw()->log(log_loc(), level::info, "blend shape {} is not export", l_name);
          continue;
        }
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

void fbx_node_sim_mesh::build_data() {
  // 如果是解算, 只需要构建mesh
  if (!dag_path.isValid()) return;
  fbx_node_transform::build_data();

  build_mesh();
}

void fbx_node_sim_mesh::build_animation(const MTime& in_time) { return; }
void fbx_node_sim_mesh::build_skin() { return; }
void fbx_node_sim_mesh::build_blend_shape() { return; }
MObject fbx_node_sim_mesh::get_skin_custer() const { return {}; }
std::vector<MDagPath> fbx_node_sim_mesh::find_joint(const MObject& in_msk) const { return {}; }
std::vector<MObject> fbx_node_sim_mesh::find_blend_shape() const { return {}; }
MObject fbx_node_sim_mesh::get_bind_post() const { return {}; }

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
  // 骨骼强制缩放为1
  node->LclScaling.Set({1.0, 1.0, 1.0});

  auto l_parent_path = dag_path;
  l_parent_path.pop();
  if (l_parent_path.hasFn(MFn::kTransform) && !l_parent_path.hasFn(MFn::kJoint)) {
    l_is_ = false;
  }
  node->InheritType.Set(l_is_ ? fbxsdk::FbxTransform::eInheritRrs : fbxsdk::FbxTransform::eInheritRSrs);
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
  scene_->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::MayaYUp);
  scene_->GetGlobalSettings().SetOriginalUpAxis(FbxAxisSystem::MayaYUp);

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
  // try {
  init();
  build_tree(in_vector, in_sim_vector);
  build_data();

  // 初始化解算分解器
  std::int64_t l_size = in_end.value() - in_begin.value() + 1;
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
  // } catch (const std::exception& in_error) {
  //   auto l_str = boost::diagnostic_information(in_error);
  //   MGlobal::displayError(conv::to_ms(l_str));
  //   log_error(logger_, fmt::format("导出文件 {} 错误 {}", path_, l_str));
  //   return;
  // }

  // 写出全部 bindpose
  // build post
  {
    auto* l_post = FbxPose::Create(scene_, fmt::format("{}_post", "main").c_str());
    l_post->SetIsBindPose(true);
    std::vector<fbx_node_ptr> post_add{};

    std::function<bool(fbx_tree_t::iterator)> l_iter{};
    l_iter = [&](fbx_tree_t::iterator in_parent) -> bool {
      bool l_r{};
      for (auto l_it = in_parent.begin(); l_it != in_parent.end(); ++l_it) {
        auto l_sub_has = l_iter(l_it);
        if (l_sub_has) {
          l_r |= true;
          l_post->Add((*l_it)->node, (*l_it)->node->EvaluateGlobalTransform());
        }
      }
      return l_r;
    };
    l_iter(tree_.begin());
    scene_->AddPose(l_post);
  }
  logger_->flush();
}

void fbx_write::write(MDagPath in_cam_path, const MTime& in_begin, const MTime& in_end) {
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

  auto l_fbx_cam = std::make_shared<fbx_write_ns::fbx_node_cam>(
      in_cam_path, FbxNode::Create(scene_, get_node_name(in_cam_path).c_str())
  );
  scene_->GetRootNode()->AddChild(l_fbx_cam->node);
  l_fbx_cam->extra_data_.logger_ = logger_;
  l_fbx_cam->build_data();
  logger_->info("开始导出camera动画 {}", in_cam_path);

  if (export_anim_) {
    for (auto l_time = in_begin; l_time <= in_end; ++l_time) {
      MAnimControl::setCurrentTime(l_time);
      l_fbx_cam->build_animation(l_time);
    }
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
            std::shared_ptr<fbx_node_mesh_t> l_mesh{};
            if (in_is_sim) {
              l_mesh = std::make_shared<fbx_node_sim_mesh_t>(
                  l_sub_path, FbxNode::Create(scene_, get_node_name(l_sub_path).c_str())
              );
            } else {
              l_mesh = std::make_shared<fbx_node_mesh_t>(
                  l_sub_path, FbxNode::Create(scene_, get_node_name(l_sub_path).c_str())
              );
            }
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

        fbx_tree_t::iterator l_node_iter{};
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
      (*i)->extra_data_.tree_         = &tree_;
      (*i)->extra_data_.material_map_ = &material_map_;
      (*i)->extra_data_.logger_       = logger_;
      l_iter_init(i);
    }
  };
  l_iter_init(tree_.begin());

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
} // namespace doodle::maya_plug