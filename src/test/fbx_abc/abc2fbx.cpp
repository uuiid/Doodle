//
// Created by td_main on 2023/9/12.
//

#include <doodle_lib/doodle_lib_all.h>

#include <Alembic/Abc/ArchiveInfo.h>
#include <Alembic/Abc/Foundation.h>
#include <Alembic/Abc/IArchive.h>
#include <Alembic/Abc/TypedArraySample.h>
#include <Alembic/AbcCoreAbstract/TimeSampling.h>
#include <Alembic/AbcCoreHDF5/All.h>
#include <Alembic/AbcCoreOgawa/All.h>
#include <Alembic/AbcCoreOgawa/ReadWrite.h>
#include <Alembic/AbcGeom/All.h>
#include <Alembic/AbcGeom/FaceSetExclusivity.h>
#include <Alembic/AbcGeom/Foundation.h>
#include <Alembic/AbcGeom/GeometryScope.h>
#include <Alembic/AbcGeom/IGeomBase.h>
#include <Alembic/AbcGeom/IPolyMesh.h>
#include <Alembic/AbcGeom/IXform.h>
#include <Alembic/AbcGeom/OFaceSet.h>
#include <Alembic/AbcGeom/OGeomParam.h>
#include <Alembic/AbcGeom/OPolyMesh.h>
#include <Alembic/AbcGeom/OXform.h>
#include <Alembic/AbcGeom/XformOp.h>
#include <Alembic/Util/PlainOldDataType.h>
#include <argh.h>
#include <fbxsdk.h>
namespace doodle {

FbxNode* CreateSkeleton(FbxScene* pScene, const char* pName) {
  // Create skeleton root.
  FbxString lRootName(pName);
  lRootName += "Root";
  FbxSkeleton* lSkeletonRootAttribute = FbxSkeleton::Create(pScene, pName);
  lSkeletonRootAttribute->SetSkeletonType(FbxSkeleton::eRoot);
  FbxNode* lSkeletonRoot = FbxNode::Create(pScene, lRootName.Buffer());
  lSkeletonRoot->SetNodeAttribute(lSkeletonRootAttribute);
  lSkeletonRoot->LclTranslation.Set(FbxVector4(0.0, -40.0, 0.0));

  // Create skeleton first limb node.
  FbxString lLimbNodeName1(pName);
  lLimbNodeName1 += "LimbNode1";
  FbxSkeleton* lSkeletonLimbNodeAttribute1 = FbxSkeleton::Create(pScene, lLimbNodeName1);
  lSkeletonLimbNodeAttribute1->SetSkeletonType(FbxSkeleton::eLimbNode);
  lSkeletonLimbNodeAttribute1->Size.Set(1.0);
  FbxNode* lSkeletonLimbNode1 = FbxNode::Create(pScene, lLimbNodeName1.Buffer());
  lSkeletonLimbNode1->SetNodeAttribute(lSkeletonLimbNodeAttribute1);
  lSkeletonLimbNode1->LclTranslation.Set(FbxVector4(0.0, 40.0, 0.0));

  // Create skeleton second limb node.
  FbxString lLimbNodeName2(pName);
  lLimbNodeName2 += "LimbNode2";
  FbxSkeleton* lSkeletonLimbNodeAttribute2 = FbxSkeleton::Create(pScene, lLimbNodeName2);
  lSkeletonLimbNodeAttribute2->SetSkeletonType(FbxSkeleton::eLimbNode);
  lSkeletonLimbNodeAttribute2->Size.Set(1.0);
  FbxNode* lSkeletonLimbNode2 = FbxNode::Create(pScene, lLimbNodeName2.Buffer());
  lSkeletonLimbNode2->SetNodeAttribute(lSkeletonLimbNodeAttribute2);
  lSkeletonLimbNode2->LclTranslation.Set(FbxVector4(0.0, 40.0, 0.0));

  // Build skeleton node hierarchy.
  lSkeletonRoot->AddChild(lSkeletonLimbNode1);
  lSkeletonLimbNode1->AddChild(lSkeletonLimbNode2);

  return lSkeletonRoot;
}

void LinkPatchToSkeleton(FbxScene* pScene, FbxNode* pPatch, FbxNode* pSkeletonRoot) {
  int i, j;
  FbxAMatrix lXMatrix;

  FbxNode* lRoot             = pSkeletonRoot;
  FbxNode* lLimbNode1        = pSkeletonRoot->GetChild(0);
  FbxNode* lLimbNode2        = lLimbNode1->GetChild(0);

  // Bottom section of cylinder is clustered to skeleton root.
  FbxCluster* lClusterToRoot = FbxCluster::Create(pScene, "");
  lClusterToRoot->SetLink(lRoot);
  lClusterToRoot->SetLinkMode(FbxCluster::eTotalOne);
  for (i = 0; i < 4; ++i)
    for (j = 0; j < 4; ++j) lClusterToRoot->AddControlPointIndex(4 * i + j, 1.0 - 0.25 * i);

  // Center section of cylinder is clustered to skeleton limb node.
  FbxCluster* lClusterToLimbNode1 = FbxCluster::Create(pScene, "");
  lClusterToLimbNode1->SetLink(lLimbNode1);
  lClusterToLimbNode1->SetLinkMode(FbxCluster::eTotalOne);

  for (i = 1; i < 6; ++i)
    for (j = 0; j < 4; ++j) lClusterToLimbNode1->AddControlPointIndex(4 * i + j, (i == 1 || i == 5 ? 0.25 : 0.50));

  // Top section of cylinder is clustered to skeleton limb.

  FbxCluster* lClusterToLimbNode2 = FbxCluster::Create(pScene, "");
  lClusterToLimbNode2->SetLink(lLimbNode2);
  lClusterToLimbNode2->SetLinkMode(FbxCluster::eTotalOne);

  for (i = 3; i < 7; ++i)
    for (j = 0; j < 4; ++j) lClusterToLimbNode2->AddControlPointIndex(4 * i + j, 0.25 * (i - 2));

  // Now we have the Patch and the skeleton correctly positioned,
  // set the Transform and TransformLink matrix accordingly.
  //  FbxScene* lScene = pPatch->GetScene();
  //  if (lScene) lXMatrix = pPatch->EvaluateGlobalTransform();
  //
  //  lClusterToRoot->SetTransformMatrix(lXMatrix);
  //  lClusterToLimbNode1->SetTransformMatrix(lXMatrix);
  //  lClusterToLimbNode2->SetTransformMatrix(lXMatrix);
  //
  //  if (lScene) lXMatrix = lRoot->EvaluateGlobalTransform();
  //  lClusterToRoot->SetTransformLinkMatrix(lXMatrix);
  //
  //  if (lScene) lXMatrix = lLimbNode1->EvaluateGlobalTransform();
  //  lClusterToLimbNode1->SetTransformLinkMatrix(lXMatrix);
  //
  //  if (lScene) lXMatrix = lLimbNode2->EvaluateGlobalTransform();
  //  lClusterToLimbNode2->SetTransformLinkMatrix(lXMatrix);

  // Add the clusters to the patch by creating a skin and adding those clusters to that skin.
  // After add that skin.

  FbxGeometry* lPatchAttribute = (FbxGeometry*)pPatch->GetNodeAttribute();
  FbxSkin* lSkin               = FbxSkin::Create(pScene, "");
  lSkin->AddCluster(lClusterToRoot);
  lSkin->AddCluster(lClusterToLimbNode1);
  lSkin->AddCluster(lClusterToLimbNode2);
  lPatchAttribute->AddDeformer(lSkin);
}

void AddNodeRecursively(FbxArray<FbxNode*>& pNodeArray, FbxNode* pNode) {
  if (pNode) {
    AddNodeRecursively(pNodeArray, pNode->GetParent());

    if (pNodeArray.Find(pNode) == -1) {
      // Node not in the list, add it
      pNodeArray.Add(pNode);
    }
  }
}

void write_fbx_impl(FbxScene* in_scene, const FSys::path& in_fbx_path) {
  auto* l_manager = in_scene->GetFbxManager();
  std::shared_ptr<FbxExporter> l_exporter{
      FbxExporter::Create(in_scene->GetFbxManager(), ""), [](FbxExporter* in_exporter) { in_exporter->Destroy(); }
  };
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_MATERIAL, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_TEXTURE, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_SHAPE, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GOBO, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_ANIMATION, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);
  l_manager->GetIOSettings()->SetBoolProp(EXP_ASCIIFBX, false);

  if (!l_exporter->Initialize(
          in_fbx_path.string().c_str(), l_manager->GetIOPluginRegistry()->GetNativeWriterFormat(),
          in_scene->GetFbxManager()->GetIOSettings()
      )) {
    DOODLE_LOG_ERROR("fbx exporter Initialize error: {}", l_exporter->GetStatus().GetErrorString());
  }
  l_exporter->Export(in_scene);
}
void StoreBindPose(FbxScene* pScene, FbxNode* pPatch) {
  // In the bind pose, we must store all the link's global matrix at the time of the bind.
  // Plus, we must store all the parent(s) global matrix of a link, even if they are not
  // themselves deforming any model.

  // In this example, since there is only one model deformed, we don't need walk through
  // the scene
  //

  // Now list the all the link involve in the patch deformation
  FbxArray<FbxNode*> lClusteredFbxNodes;
  int i, j;

  if (pPatch && pPatch->GetNodeAttribute()) {
    int lSkinCount    = 0;
    int lClusterCount = 0;
    switch (pPatch->GetNodeAttribute()->GetAttributeType()) {
      default:
        break;
      case FbxNodeAttribute::eMesh:
      case FbxNodeAttribute::eNurbs:
      case FbxNodeAttribute::ePatch:

        lSkinCount = ((FbxGeometry*)pPatch->GetNodeAttribute())->GetDeformerCount(FbxDeformer::eSkin);
        // Go through all the skins and count them
        // then go through each skin and get their cluster count
        for (i = 0; i < lSkinCount; ++i) {
          FbxSkin* lSkin = (FbxSkin*)((FbxGeometry*)pPatch->GetNodeAttribute())->GetDeformer(i, FbxDeformer::eSkin);
          lClusterCount += lSkin->GetClusterCount();
        }
        break;
    }
    // if we found some clusters we must add the node
    if (lClusterCount) {
      // Again, go through all the skins get each cluster link and add them
      for (i = 0; i < lSkinCount; ++i) {
        FbxSkin* lSkin = (FbxSkin*)((FbxGeometry*)pPatch->GetNodeAttribute())->GetDeformer(i, FbxDeformer::eSkin);
        lClusterCount  = lSkin->GetClusterCount();
        for (j = 0; j < lClusterCount; ++j) {
          FbxNode* lClusterNode = lSkin->GetCluster(j)->GetLink();
          AddNodeRecursively(lClusteredFbxNodes, lClusterNode);
        }
      }

      // Add the patch to the pose
      lClusteredFbxNodes.Add(pPatch);
    }
  }

  // Now create a bind pose with the link list
  if (lClusteredFbxNodes.GetCount()) {
    // A pose must be named. Arbitrarily use the name of the patch node.
    FbxPose* lPose = FbxPose::Create(pScene, pPatch->GetName());

    // default pose type is rest pose, so we need to set the type as bind pose
    lPose->SetIsBindPose(true);

    for (i = 0; i < lClusteredFbxNodes.GetCount(); i++) {
      FbxNode* lKFbxNode    = lClusteredFbxNodes.GetAt(i);
      FbxMatrix lBindMatrix = lKFbxNode->EvaluateGlobalTransform();

      lPose->Add(lKFbxNode, lBindMatrix);
    }

    // Add the pose to the scene
    pScene->AddPose(lPose);
  }
}
// 写入fbx
void write_fbx(const FSys::path& in_fbx_path, const Alembic::AbcGeom::IPolyMesh& in_poly) {
  std::shared_ptr<FbxManager> l_manager{FbxManager::Create(), [](FbxManager* in_manager) { in_manager->Destroy(); }};
  FbxIOSettings* l_io_settings = FbxIOSettings::Create(l_manager.get(), IOSROOT);
  l_manager->SetIOSettings(l_io_settings);
  FbxScene* l_scene{FbxScene::Create(l_manager.get(), "scene")};

  FbxDocumentInfo* l_doc_info{FbxDocumentInfo::Create(l_manager.get(), "info")};
  l_doc_info->mTitle   = "doodle fbx";
  l_doc_info->mSubject = "doodle fbx";
  l_doc_info->mAuthor  = "doodle";
  l_doc_info->Original_ApplicationVendor.Set("doodle");
  l_doc_info->Original_ApplicationName.Set("doodle");
  l_doc_info->Original_ApplicationVersion.Set("1.0.0");

  l_doc_info->LastSaved_ApplicationVendor.Set("doodle");
  l_doc_info->LastSaved_ApplicationName.Set("doodle");
  l_doc_info->LastSaved_ApplicationVersion.Set("1.0.0");

  l_scene->SetSceneInfo(l_doc_info);
  //  l_scene->GetSrcObject<FbxMesh>(0);

  l_scene->GetGlobalSettings().SetSystemUnit(FbxSystemUnit::cm);
  auto anim_stack = FbxAnimStack::Create(l_scene, "anim_stack");
  auto anim_layer = FbxAnimLayer::Create(l_scene, "anim_layer");
  anim_stack->AddMember(anim_layer);

  auto* l_mesh_node = FbxNode::Create(l_scene, "node");
  // 写出网格
  auto* l_mesh      = FbxMesh::Create(l_scene, "mesh");

  {  // 先写出节点
    l_scene->GetRootNode()->AddChild(l_mesh_node);

    const auto& l_sample       = in_poly.getSchema();
    // 写出顶点
    auto l_sample_data         = l_sample.getValue();

    auto l_sample_face_indices = l_sample_data.getFaceIndices();
    auto l_sample_face_counts  = l_sample_data.getFaceCounts();
    auto l_sample_pos          = l_sample_data.getPositions();

    DOODLE_LOG_INFO("pos size: {}", l_sample_pos->size());
    l_mesh->InitControlPoints(l_sample_pos->size());
    auto* l_pos_list = l_mesh->GetControlPoints();
    for (std::size_t j = 0; j < l_sample_pos->size(); ++j) {
      const auto& l_pos = (*l_sample_pos)[j];
      l_pos_list[j]     = FbxVector4{l_pos.x, l_pos.y, l_pos.z};
      //    DOODLE_LOG_INFO("pos: {} {} {}", l_pos.x, l_pos.y, l_pos.z);
    }
    DOODLE_LOG_INFO("face size: {} {}", l_sample_face_counts->size(), l_sample_face_indices->size());
    //  写出多边形连接
    std::size_t l_index{};
    for (std::size_t j = 0; j < l_sample_face_counts->size(); ++j) {
      l_mesh->BeginPolygon();
      // 在abc 多边形旋转顺序是相反的, 所以直接反向
      for (std::int32_t k = (*l_sample_face_counts)[j] - 1; k > -1; --k) {
        //      DOODLE_LOG_INFO("face index: {}", l_index + k);
        l_mesh->AddPolygon((*l_sample_face_indices)[l_index + k]);
      }
      l_index += (*l_sample_face_counts)[j];
      l_mesh->EndPolygon();
    }

    auto* l_layer = l_mesh->GetLayer(0);
    if (!l_layer) {
      l_mesh->CreateLayer();
      l_layer = l_mesh->GetLayer(0);
    }

    auto* l_layer_element_normal = FbxLayerElementNormal::Create(l_mesh, "");
    l_layer_element_normal->SetMappingMode(FbxLayerElement::eByControlPoint);
    l_layer_element_normal->SetReferenceMode(FbxLayerElement::eDirect);
    auto* l_layer_element_tangent = FbxLayerElementTangent::Create(l_mesh, "");
    l_layer_element_tangent->SetMappingMode(FbxLayerElement::eByControlPoint);
    l_layer_element_tangent->SetReferenceMode(FbxLayerElement::eDirect);
    auto* l_layer_element_binormal = FbxLayerElementBinormal::Create(l_mesh, "");
    l_layer_element_binormal->SetMappingMode(FbxLayerElement::eByControlPoint);
    l_layer_element_binormal->SetReferenceMode(FbxLayerElement::eDirect);
    // set normal
    for (auto i = 0; i < l_sample_face_indices->size(); i += 3) {
      //    auto l_normal = (l_pos_list[i + 1] - l_pos_list[i]).CrossProduct(l_pos_list[i + 2] - l_pos_list[i]);
      //    l_normal.Normalize();
      //    l_layer_element_normal->GetDirectArray().Add(FbxVector4{l_normal[0], l_normal[1], l_normal[2]});
      //    l_layer_element_normal->GetIndexArray().Add(i);
      //    l_layer_element_normal->GetIndexArray().Add(i + 1);
      //    l_layer_element_normal->GetIndexArray().Add(i + 2);
      l_layer_element_normal->GetDirectArray().Add(FbxVector4{});
      l_layer_element_tangent->GetDirectArray().Add(FbxVector4{});
      l_layer_element_binormal->GetDirectArray().Add(FbxVector4{});
    }

    l_layer->SetNormals(l_layer_element_normal);
    l_layer->SetTangents(l_layer_element_tangent);
    l_layer->SetBinormals(l_layer_element_binormal);

    auto* l_fbx_mat = FbxSurfaceLambert::Create(l_scene, "Fbx Default Material");
    l_mesh_node->SetNodeAttribute(l_mesh);
    l_mesh_node->AddMaterial(l_fbx_mat);
  }
  //  FbxNode* lSkeletonRoot = CreateSkeleton(l_scene, "skeleton");
  //  l_scene->GetRootNode()->AddChild(lSkeletonRoot);
  //  LinkPatchToSkeleton(l_scene, l_mesh_node, lSkeletonRoot);
  //  StoreBindPose(l_scene, l_mesh_node);
  //  return write_fbx_impl(l_scene, in_fbx_path);

  const auto l_for_size = 2;
  std::vector<FbxNode*> l_bones{};
  //  auto l_name = "skeleton_";
  //
  //  FbxString lRootName(l_name);
  //  lRootName += "Root";
  //  FbxSkeleton* lSkeletonRootAttribute = FbxSkeleton::Create(l_scene, l_name);
  //  lSkeletonRootAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
  //  FbxNode* lSkeletonRoot = FbxNode::Create(l_scene, lRootName.Buffer());
  //  lSkeletonRoot->SetNodeAttribute(lSkeletonRootAttribute);
  //  lSkeletonRoot->LclTranslation.Set(FbxVector4(0.0, -40.0, 0.0));
  //  l_scene->GetRootNode()->AddChild(lSkeletonRoot);

  //
  //  FbxString lLimbNodeName1(l_name);
  //  lLimbNodeName1 += "LimbNode1";
  //  FbxSkeleton* lSkeletonLimbNodeAttribute1 = FbxSkeleton::Create(l_scene,lLimbNodeName1);
  //  lSkeletonLimbNodeAttribute1->SetSkeletonType(FbxSkeleton::eLimbNode);
  //  lSkeletonLimbNodeAttribute1->Size.Set(1.0);
  //  FbxNode* lSkeletonLimbNode1 = FbxNode::Create(l_scene,lLimbNodeName1.Buffer());
  //  lSkeletonLimbNode1->SetNodeAttribute(lSkeletonLimbNodeAttribute1);
  //  lSkeletonLimbNode1->LclTranslation.Set(FbxVector4(0.0, 40.0, 0.0));
  //  lSkeletonRoot->AddChild(lSkeletonLimbNode1);

  for (auto i = 0; i < l_for_size; ++i) {  // 再写 骨骼

    // #ifdef DOODLE_1001
    auto* l_sk_attr = FbxSkeleton::Create(l_scene, fmt::format("skeleton_{}", i).c_str());
    l_sk_attr->SetSkeletonType(FbxSkeleton::eLimbNode);
    //    l_sk_attr->Size.Set(1.0);

    auto* l_bone_node = FbxNode::Create(l_scene, fmt::format("skeleton_{}", i).c_str());
    l_bone_node->SetNodeAttribute(l_sk_attr);
    l_bone_node->SetRotationOrder(FbxNode::eSourcePivot, eEulerXYZ);
    auto l_box = in_poly.getSchema().getSelfBoundsProperty().getValue().center();
    l_bone_node->LclTranslation.Set(FbxDouble3{l_box.x, l_box.y, l_box.z});
    l_bones.emplace_back(l_bone_node);
    l_scene->GetRootNode()->AddChild(l_bone_node);
    // #endif
  }
  //  return write_fbx_impl(l_scene, in_fbx_path);

  {  // 写一个皮肤变形器
    auto* l_sk = FbxSkin::Create(l_scene, "");
    //    dynamic_cast<FbxGeometry*>(l_mesh_node->GetNodeAttribute())->AddDeformer(l_sk);
    //    FbxGeometry* lPatchAttribute = (FbxGeometry*)l_mesh_node->GetNodeAttribute();
    //    lPatchAttribute->AddDeformer(l_sk);

    for (int l = 0; l < l_for_size; ++l) {
      auto* l_cluster = FbxCluster::Create(l_scene, "");
      l_cluster->SetLink(l_bones[l]);
      l_cluster->SetLinkMode(FbxCluster::eTotalOne);
      l_cluster->AddControlPointIndex(l, 1.0);

      l_cluster->SetTransformMatrix(l_mesh_node->EvaluateGlobalTransform());
      l_cluster->SetTransformLinkMatrix(l_bones[l]->EvaluateGlobalTransform());
      if (!l_sk->AddCluster(l_cluster)) DOODLE_LOG_ERROR("add cluster error: {}", l);
    }
    l_mesh->AddDeformer(l_sk);
    DOODLE_LOG_INFO("sk cluster size {}", l_sk->GetClusterCount());
    // 绑定post
    auto* l_post = FbxPose::Create(l_scene, "bind_pose");
    l_post->SetIsBindPose(true);
    for (auto* l_bone : l_bones) {
      if (l_post->Add(l_bone, l_bone->EvaluateGlobalTransform()) == -1) {
        DOODLE_LOG_ERROR("add bone error: {}", l_bone->GetName());
      }
    }
    l_post->Add(l_mesh_node, l_mesh_node->EvaluateGlobalTransform());

    l_scene->AddPose(l_post);
  }

  {  // 写一个混变
    auto* l_blend         = FbxBlendShape::Create(l_scene, "blend");
    auto* l_blend_channel = FbxBlendShapeChannel::Create(l_scene, "blend_channel");

    auto* l_shape         = FbxShape::Create(l_scene, "shape");
    l_shape->InitControlPoints(l_mesh->GetControlPointsCount());
    auto* l_shape_pos = l_shape->GetControlPoints();
    for (auto i = 0; i < l_mesh->GetControlPointsCount(); ++i) {
      l_shape_pos[i] = l_mesh->GetControlPointAt(i);
    }
    l_blend_channel->AddTargetShape(l_shape);
    l_blend->AddBlendShapeChannel(l_blend_channel);
    l_mesh->AddDeformer(l_blend);
  }

  return write_fbx_impl(l_scene, in_fbx_path);
}

void run(const FSys::path& in_abc_path) {
  Alembic::Abc::IArchive l_archive{Alembic::AbcCoreOgawa::ReadArchive(), in_abc_path.string()};
  auto l_top = l_archive.getTop();
  if (l_top.getNumChildren() == 0) {
    DOODLE_LOG_ERROR("abc file is empty");
    return;
  }

  for (auto i = 0; i < l_top.getNumChildren(); ++i) {
    auto l_child = l_top.getChild(i);
    l_child.getHeader();
    if (Alembic::AbcGeom::IXform::matches(l_child.getHeader())) {
      auto l_xform = Alembic::AbcGeom::IXform{l_child};
      DOODLE_LOG_INFO("xform {}", l_xform.getName());
      for (int l = 0; l < l_xform.getNumChildren(); ++l) {
        if (Alembic::AbcGeom::IPolyMesh::matches(l_xform.getChild(l).getHeader())) {
          auto l_poly = Alembic::AbcGeom::IPolyMesh{l_xform.getChild(l)};
          DOODLE_LOG_INFO("poly {}", l_poly.getName());
          // print bounds
          auto l_bounds = l_poly.getSchema().getSelfBoundsProperty().getValue();
          DOODLE_LOG_INFO(
              "bounds: [{} {} {}, {} {} {}] {} {} {}", l_bounds.min.x, l_bounds.min.y, l_bounds.min.z, l_bounds.max.x,
              l_bounds.max.y, l_bounds.max.z, l_bounds.size().x, l_bounds.size().y, l_bounds.size().z
          );
          l_poly.getSchema().getSelfBoundsProperty().getValue();
          write_fbx(in_abc_path.parent_path() / (l_poly.getName() + ".fbx"), l_poly);
          return;
        }
      }
    }
  }
}

}  // namespace doodle

int main(int argc, char* argv[]) {
  using namespace doodle;
  argh::parser l_parser(argc, argv, argh::parser::PREFER_PARAM_FOR_UNREG_OPTION);
  if (auto l_ss = l_parser("--abc"); l_ss) {
    FSys::path l_path{FSys::from_quotation_marks(l_ss.str())};
    l_path = l_path.make_preferred();
    run(l_path);
  } else {
    DOODLE_LOG_ERROR("abc path is empty");
  }
  return 0;
}