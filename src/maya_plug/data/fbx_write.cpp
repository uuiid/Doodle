//
// Created by td_main on 2023/10/16.
//

#include "fbx_write.h"

#include <boost/lambda2.hpp>

#include <maya_plug/data/dagpath_cmp.h>
#include <maya_plug/data/maya_conv_str.h>
#include <maya_plug/fmt/fmt_dag_path.h>
#include <maya_plug/fmt/fmt_select_list.h>

#include <fbxsdk.h>
#include <maya/MAnimControl.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatArray.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFnTransform.h>
#include <maya/MGlobal.h>
#include <maya/MItDag.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItGeometry.h>
#include <maya/MItMeshFaceVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MTime.h>
#include <treehh/tree.hh>

namespace doodle::maya_plug {
namespace fbx_write_ns {
void fbx_node::build_node(const fbx_tree_t& in_tree) {
  std::call_once(flag_, [&]() { build_data(in_tree); });
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

  auto l_loc = l_transform.getTranslation(MSpace::kTransform, &l_status);
  maya_chick(l_status);
  node->LclTranslation.Set({l_loc.x, l_loc.y, l_loc.z});

  // rot
  {
    auto l_rot_x = get_plug(in_path.node(), "rotateX").asMAngle(&l_status);
    maya_chick(l_status);
    auto l_rot_y = get_plug(in_path.node(), "rotateY").asMAngle(&l_status);
    maya_chick(l_status);
    auto l_rot_z = get_plug(in_path.node(), "rotateZ").asMAngle(&l_status);
    maya_chick(l_status);

    node->LclRotation.Set({l_rot_x.asDegrees(), l_rot_y.asDegrees(), l_rot_z.asDegrees()});
  }

  std::double_t l_scale[3]{};
  l_transform.getScale(l_scale);
  node->LclScaling.Set({l_scale[0], l_scale[1], l_scale[2]});
  node->ScalingMax.Set({});
}

////
void fbx_node_transform::build_data(const fbx_tree_t& in_tree) {
  build_node_transform(dag_path);
  MFnTransform l_transform{dag_path};
  auto l_attr_null = FbxNull::Create(node->GetScene(), l_transform.name().asChar());
  l_attr_null->Look.Set(FbxNull::eNone);
  node->SetNodeAttribute(l_attr_null);
}

void fbx_node_transform::build_animation(const fbx_tree_t& in_tree, const MTime& in_time) {
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

  // rot x
  {
    auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_rot_x     = get_plug(dag_path.node(), "rotateX").asMAngle(&l_status);
    maya_chick(l_status);
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_rot_x.asDegrees());
    l_anim_curve->KeyModifyEnd();
  }

  // rot y
  {
    auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_rot_y     = get_plug(dag_path.node(), "rotateY").asMAngle(&l_status);
    maya_chick(l_status);
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_rot_y.asDegrees());
    l_anim_curve->KeyModifyEnd();
  }

  // rot z
  {
    auto* l_anim_curve = node->LclRotation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    l_anim_curve->KeyModifyBegin();
    auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
    auto l_rot_z     = get_plug(dag_path.node(), "rotateZ").asMAngle(&l_status);
    maya_chick(l_status);
    l_anim_curve->KeySet(l_key_index, l_fbx_time, l_rot_z.asDegrees());
    l_anim_curve->KeyModifyEnd();
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

void fbx_node_mesh::build_data(const doodle::maya_plug::fbx_write_ns::fbx_tree_t& in_tree) {
  if (!dag_path.isValid()) return;
  fbx_node_transform::build_data(in_tree);

  build_mesh();
  build_skin(in_tree);
  build_blend_shape();
}
void fbx_node_mesh::build_mesh() {
  if (!dag_path.hasFn(MFn::kMesh)) {
    //      log_info(fmt::format("{} is not mesh", get_node_name(in_mesh)));
    return;
  }
  log_info(fmt::format("build mesh {}", dag_path));

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
      l_mat_layer->SetMappingMode(FbxLayerElement::eAllSame);
      l_mat_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);
      l_mat_layer->GetIndexArray().Add(0);
      auto* l_mat_surface = FbxSurfaceLambert::Create(
          node->GetScene(), get_node_name(details::shading_engine_to_mat(l_mats.front())).c_str()
      );
      node->AddMaterial(l_mat_surface);
    } else {
      l_mat_layer->SetMappingMode(FbxLayerElement::eByPolygon);
      l_mat_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);
      MFnSet l_fn_set{};
      l_mat_ids.resize(l_fn_mesh.numPolygons());
      MStatus l_status{};
      for (auto l_mat : l_mats) {
        auto* l_mat_surface =
            FbxSurfaceLambert::Create(node->GetScene(), get_node_name(details::shading_engine_to_mat(l_mat)).c_str());
        auto l_mat_index = node->AddMaterial(l_mat_surface);
        maya_chick(l_fn_set.setObject(l_mat));

        MSelectionList l_list{};
        maya_chick(l_fn_set.getMembers(l_list, false));
        MDagPath l_path{};
        MObject l_comp{};
        for (MItSelectionList l_it_geo{l_list}; !l_it_geo.isDone(); l_it_geo.next()) {
          maya_chick(l_it_geo.getDagPath(l_path, l_comp));
          if (l_path == dag_path) {
            if (l_comp.hasFn(MFn::kMeshPolygonComponent)) {
              MFnSingleIndexedComponent l_fn_comp{l_comp, &l_status};
              maya_chick(l_status);
              for (auto i = 0; i < l_fn_comp.elementCount(); ++i) {
                auto l_index = l_fn_comp.element(i, &l_status);
                maya_chick(l_status);
                if (l_index >= l_mat_ids.size()) {
                  log_error(fmt::format("mat index out of range {} {}", l_index, l_mat_ids.size()));
                  continue;
                }
                l_mat_ids[l_index] = l_mat_index;
              }
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

  {
    // get uv set names
    MStringArray l_uv_set_names{};
    maya_chick(l_fn_mesh.getUVSetNames(l_uv_set_names));
    for (auto i = 0; i < l_uv_set_names.length(); ++i) {
      log_info(fmt::format("uv set name: {}", l_uv_set_names[i]));
      auto* l_layer = mesh->CreateElementUV(l_uv_set_names[i].asChar());
      l_layer->SetMappingMode(FbxLayerElement::eByPolygonVertex);
      l_layer->SetReferenceMode(FbxLayerElement::eIndexToDirect);
      // for maya uv
      MFloatArray l_u{};
      MFloatArray l_v{};
      l_fn_mesh.getUVs(l_u, l_v, &l_uv_set_names[i]);
      for (auto j = 0; j < l_u.length(); ++j) {
        l_layer->GetDirectArray().Add(FbxVector2{l_u[j], l_v[j]});
      }

      auto l_face_count = l_fn_mesh.numPolygons();
      for (auto k = 0; k < l_face_count; ++k) {
        MIntArray l_vert_list{};
        maya_chick(l_fn_mesh.getPolygonVertices(k, l_vert_list));
        for (auto j = 0; j < l_vert_list.length(); ++j) {
          std::int32_t l_uv_id{};
          maya_chick(l_fn_mesh.getPolygonUVid(k, j, l_uv_id, &l_uv_set_names[i]));
          l_layer->GetIndexArray().Add(l_uv_id);
        }
      }
    }
  }

  // normals
  {
    auto l_layer = mesh->CreateElementNormal();
    l_layer->SetMappingMode(FbxLayerElement::eByPolygonVertex);
    l_layer->SetReferenceMode(FbxLayerElement::eDirect);

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
    l_layer->SetMappingMode(FbxLayerElement::eByEdge);
    l_layer->SetReferenceMode(FbxLayerElement::eDirect);

    for (auto i = 0; i < l_fn_mesh.numEdges(); ++i) {
      l_layer->GetDirectArray().Add(l_fn_mesh.isEdgeSmooth(i));
    }
  }
}

void fbx_node_mesh::build_skin(const fbx_tree_t& in_tree) {
  if (mesh == nullptr) {
    log_error(fmt::format(" {} is not mesh", dag_path));
    return;
  }
  auto l_skin_obj = get_skin_custer();
  auto* l_sk      = FbxSkin::Create(node->GetScene(), get_node_name(l_skin_obj).c_str());
  mesh->AddDeformer(l_sk);

  auto l_joint_list = find_joint(l_skin_obj);

  MFnSkinCluster l_skin_cluster{l_skin_obj};
  auto l_skinning_method = get_plug(l_skin_obj, "skinningMethod").asInt();
  l_sk->SetSkinningType(static_cast<FbxSkin::EType>(l_skinning_method + 1));

  std::map<MDagPath, fbx_node_ptr, details::cmp_dag> l_dag_tree_map{};
  for (auto l_it = in_tree.begin(); l_it != in_tree.end(); ++l_it) {
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
    l_cluster->SetTransformMatrix(node->EvaluateGlobalTransform());
    l_cluster->SetTransformLinkMatrix(l_joint->node->EvaluateGlobalTransform());
    l_dag_fbx_map.emplace(i, l_cluster);
    if (!l_sk->AddCluster(l_cluster)) {
      log_error(fmt::format("add cluster error: {}", node->GetName()));
    }
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

    l_iter(in_tree.begin());

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

  MFnBlendShapeDeformer l_blend_shape{};
  auto l_fbx_bl = FbxBlendShape::Create(node->GetScene(), fmt::format("{}", get_node_name(l_bls[0])).c_str());
  mesh->AddDeformer(l_fbx_bl);
  FbxProperty::Create(l_fbx_bl->RootProperty, FbxStringDT, "RootGroup");

  MStatus l_status{};
  for (auto&& i : l_bls) {
    maya_chick(l_blend_shape.setObject(i));

    auto l_input_target_plug_1 = get_plug(i, "inputTarget").elementByPhysicalIndex(0, &l_status);
    maya_chick(l_status);
    auto l_input_target_group_array = l_input_target_plug_1.child(0, &l_status);
    maya_chick(l_status);
    //    std::cout << fmt::format("get plug {}", l_input_target_group_array.name()) << std::endl;
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
      //      std::cout << fmt::format(
      //                       "{} info {}: {}|{}: {}", j, l_input_point_target.name(), l_input_point_target.info(),
      //                       l_input_components_target.name(), l_input_components_target.info()
      //                   )
      //                << std::endl;
      auto l_input_point_target_data_handle = l_input_point_target.asMDataHandle(&l_status);
      maya_chick(l_status);
      auto l_input_components_target_data_handle = l_input_components_target.asMDataHandle(&l_status);
      maya_chick(l_status);

      MFnPointArrayData l_point_data{l_input_point_target_data_handle.data(), &l_status};
      maya_chick(l_status);
      if (l_point_data.length() == 0) {
        //        log_info(fmt::format("blend shape {} point data length == 0", get_node_name(i)));
        continue;
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
      std::copy(mesh->GetControlPoints(), mesh->GetControlPoints() + mesh->GetControlPointsCount(), l_fbx_points);
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

void fbx_node_mesh::build_animation(const fbx_tree_t& in_tree, const MTime& in_time) {
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

void fbx_node_joint::build_data(const fbx_tree_t& in_tree) {
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
    const std::vector<MDagPath>& in_vector, const MTime& in_begin, const MTime& in_end, const FSys::path& in_path
) {
  path_            = in_path;
  auto* anim_stack = scene_->GetCurrentAnimationStack();
  FbxTime l_fbx_begin{};
  l_fbx_begin.SetFrame(in_begin.value(), fbx_write_ns::fbx_node::maya_to_fbx_time(in_begin.unit()));
  anim_stack->LocalStart = l_fbx_begin;
  FbxTime l_fbx_end{};
  l_fbx_end.SetFrame(in_end.value(), fbx_write_ns::fbx_node::maya_to_fbx_time(in_end.unit()));
  anim_stack->LocalStop = l_fbx_end;

  MAnimControl::setCurrentTime(in_begin);

  init();
  build_tree(in_vector);

  try {
    build_data();
  } catch (const maya_error& in_error) {
    MGlobal::displayError(conv::to_ms(boost::diagnostic_information(in_error)));
    return;
  }

  for (auto l_time = in_begin; l_time <= in_end; ++l_time) {
    MAnimControl::setCurrentTime(l_time);
    build_animation(l_time);
  }
  write_end();
}

void fbx_write::write(
    const MSelectionList& in_vector, const MTime& in_begin, const MTime& in_end, const FSys::path& in_path
) {
  std::vector<MDagPath> l_objs{};
  MDagPath l_path{};
  for (MItSelectionList l_it{in_vector}; !l_it.isDone(); l_it.next()) {
    maya_chick(l_it.getDagPath(l_path));
    l_objs.emplace_back(l_path);
  }
  write(l_objs, in_begin, in_end, in_path);
}

void fbx_write::write_end() {
  std::shared_ptr<FbxExporter> l_exporter{
      FbxExporter::Create(scene_->GetFbxManager(), ""), [](FbxExporter* in_exporter) { in_exporter->Destroy(); }};
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
          manager_->GetIOPluginRegistry()->GetNativeWriterFormat(),  //
          //          manager_->GetIOPluginRegistry()->FindWriterIDByDescription("FBX ascii (*.fbx)"),
          scene_->GetFbxManager()->GetIOSettings()
      )) {
    MGlobal::displayError(
        conv::to_ms(fmt::format("fbx exporter Initialize error: {}", l_exporter->GetStatus().GetErrorString()))
    );
    return;
  }
  l_exporter->Export(scene_);
}
void fbx_write::init() { tree_ = {std::make_shared<fbx_node_transform_t>(MDagPath{}, scene_->GetRootNode())}; }
void fbx_write::build_tree(const std::vector<MDagPath>& in_vector) {
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
              l_sub_path, FbxNode::Create(scene_, l_sub_path.partialPathName().asChar())
          );
          joints_ |= ranges::action::push_back(l_mesh->find_joint(l_mesh->get_skin_custer()));

          l_begin = tree_.append_child(l_begin, l_mesh);
        } else if (l_sub_path.hasFn(MFn::kJoint)) {
          l_begin = tree_.append_child(
              l_begin, std::make_shared<fbx_node_joint_t>(
                           l_sub_path, FbxNode::Create(scene_, l_sub_path.partialPathName().asChar())
                       )
          );
        } else {
          l_begin = tree_.append_child(
              l_begin, std::make_shared<fbx_node_transform_t>(
                           l_sub_path, FbxNode::Create(scene_, l_sub_path.partialPathName().asChar())
                       )
          );
        }
        node_map_.emplace(l_sub_path, *l_begin);
        l_parent_node->AddChild((*l_begin)->node);
      }
    }
  }

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
              l_begin, std::make_shared<fbx_node_joint_t>(
                           l_sub_path, FbxNode::Create(scene_, l_sub_path.partialPathName().asChar())
                       )
          );
        } else {
          l_begin = tree_.append_child(
              l_begin, std::make_shared<fbx_node_transform_t>(
                           l_sub_path, FbxNode::Create(scene_, l_sub_path.partialPathName().asChar())
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

        auto l_parent_node = (*l_tree_parent)->node;

        fbx_tree_t ::iterator l_node_iter{};
        if (l_path.hasFn(MFn::kJoint)) {
          l_node_iter = tree_.append_child(
              l_tree_parent,
              std::make_shared<fbx_node_joint_t>(l_path, FbxNode::Create(scene_, l_path.partialPathName().asChar()))
          );
        } else {
          l_node_iter = tree_.append_child(
              l_tree_parent,
              std::make_shared<fbx_node_transform_t>(l_path, FbxNode::Create(scene_, l_path.partialPathName().asChar()))
          );
        }
        l_parent_node->AddChild((*l_node_iter)->node);
        node_map_.emplace(l_path, *l_node_iter);
      }
    }
  }
}
void fbx_write::build_data() {
  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_fun{};
  l_iter_fun = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      if (!(*i)->dag_path.hasFn(MFn::kMesh)) (*i)->build_node(tree_);
      l_iter_fun(i);
    }
  };
  l_iter_fun(tree_.begin());

  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_fun2{};
  l_iter_fun2 = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      (*i)->build_node(tree_);
      l_iter_fun2(i);
    }
  };

  l_iter_fun2(tree_.begin());
}
void fbx_write::build_animation(const MTime& in_time) {
  std::function<void(const fbx_tree_t::iterator& in_iterator)> l_iter_fun{};
  l_iter_fun = [&](const fbx_tree_t::iterator& in_iterator) {
    for (auto i = in_iterator.begin(); i != in_iterator.end(); ++i) {
      (*i)->build_animation(tree_, in_time);
      l_iter_fun(i);
    }
  };
  l_iter_fun(tree_.begin());
}

fbx_write::~fbx_write() = default;
}  // namespace doodle::maya_plug
