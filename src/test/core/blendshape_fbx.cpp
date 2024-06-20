
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

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
#include <Eigen/Eigen>
#include <fbxsdk.h>
// #include <Eigen/src/Core/Matrix.h>
// #include <Eigen/src/SVD/JacobiSVD.h>
#include <filesystem>

using namespace doodle;

// 混合变形数据
struct bl_mesh_data {
  // 混合变形中的基本形状
  Eigen::VectorXd base_mesh_{};

  /**
   * 混合变形中的混变形状
   *
   * bs_mesh_[点 * 3, 混合形状数]
   *
   */
  Eigen::MatrixXd bs_mesh_{};
  // 混合变形中的权重曲线
  Eigen::MatrixXd weight_{};

  // 优化后的形状数量
  std::size_t num_blend_shape_{};
};

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
    doodle::default_logger_raw()->info("fbx exporter Initialize error: {}", l_exporter->GetStatus().GetErrorString());
  }
  l_exporter->Export(in_scene);
}

void write_fbx(
    const std::filesystem::path& in_fbx_path, const bl_mesh_data& in_poly, const std::vector<std::size_t>& in_face_size,
    const std::vector<std::int64_t>& in_face_index
) {
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
  l_scene->GetGlobalSettings().SetTimeMode(fbxsdk::FbxTime::EMode::ePAL);
  auto anim_stack = FbxAnimStack::Create(l_scene, "anim_stack");
  auto anim_layer = FbxAnimLayer::Create(l_scene, "anim_layer");
  anim_stack->AddMember(anim_layer);
  l_scene->SetCurrentAnimationStack(anim_stack);

  auto* l_mesh_node = FbxNode::Create(l_scene, "node");
  // 写出网格
  auto* l_mesh      = FbxMesh::Create(l_scene, "mesh");

  {  // 先写出节点
    l_scene->GetRootNode()->AddChild(l_mesh_node);

    //   const auto& l_sample       = in_poly.getSchema();
    //   // 写出顶点
    //   auto l_sample_data         = l_sample.getValue();

    //   auto l_sample_face_indices = l_sample_data.getFaceIndices();
    //   auto l_sample_face_counts  = l_sample_data.getFaceCounts();
    //   auto l_sample_pos          = l_sample_data.getPositions();

    doodle::default_logger_raw()->info("pos size: {}", in_poly.base_mesh_.rows() / 3);
    l_mesh->InitControlPoints(in_poly.base_mesh_.rows() / 3);
    auto* l_pos_list = l_mesh->GetControlPoints();
    for (std::size_t j = 0; j < in_poly.base_mesh_.rows() / 3; ++j) {
      auto l_poly_index = j * 3;
      l_pos_list[j]     = FbxVector4{
          (std::double_t)in_poly.base_mesh_[l_poly_index], (std::double_t)in_poly.base_mesh_[l_poly_index + 1],
          (std::double_t)in_poly.base_mesh_[l_poly_index + 2]
      };
    }
    doodle::default_logger_raw()->info("face size: {} {}", in_face_size.size(), in_face_index.size());
    //  写出多边形连接
    std::size_t l_index{};
    for (std::size_t j = 0; j < in_face_size.size(); ++j) {
      l_mesh->BeginPolygon();
      // 在abc 多边形旋转顺序是相反的, 所以直接反向
      for (std::int32_t k = in_face_size[j] - 1; k > -1; --k) {
        //      doodle::default_logger_raw()->info("face index: {}", l_index + k);
        l_mesh->AddPolygon(in_face_index[l_index + k]);
      }
      l_index += in_face_size[j];
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
    for (auto i = 0; i < in_face_index.size(); i += 3) {
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

  // //  FbxNode* lSkeletonRoot = CreateSkeleton(l_scene, "skeleton");
  // //  l_scene->GetRootNode()->AddChild(lSkeletonRoot);
  // //  LinkPatchToSkeleton(l_scene, l_mesh_node, lSkeletonRoot);
  // //  StoreBindPose(l_scene, l_mesh_node);
  // //  return write_fbx_impl(l_scene, in_fbx_path);

  // const auto l_for_size = 2;
  // std::vector<FbxNode*> l_bones{};
  // //  auto l_name = "skeleton_";
  // //
  // //  FbxString lRootName(l_name);
  // //  lRootName += "Root";
  // //  FbxSkeleton* lSkeletonRootAttribute = FbxSkeleton::Create(l_scene, l_name);
  // //  lSkeletonRootAttribute->SetSkeletonType(FbxSkeleton::eLimbNode);
  // //  FbxNode* lSkeletonRoot = FbxNode::Create(l_scene, lRootName.Buffer());
  // //  lSkeletonRoot->SetNodeAttribute(lSkeletonRootAttribute);
  // //  lSkeletonRoot->LclTranslation.Set(FbxVector4(0.0, -40.0, 0.0));
  // //  l_scene->GetRootNode()->AddChild(lSkeletonRoot);

  // //
  // //  FbxString lLimbNodeName1(l_name);
  // //  lLimbNodeName1 += "LimbNode1";
  // //  FbxSkeleton* lSkeletonLimbNodeAttribute1 = FbxSkeleton::Create(l_scene,lLimbNodeName1);
  // //  lSkeletonLimbNodeAttribute1->SetSkeletonType(FbxSkeleton::eLimbNode);
  // //  lSkeletonLimbNodeAttribute1->Size.Set(1.0);
  // //  FbxNode* lSkeletonLimbNode1 = FbxNode::Create(l_scene,lLimbNodeName1.Buffer());
  // //  lSkeletonLimbNode1->SetNodeAttribute(lSkeletonLimbNodeAttribute1);
  // //  lSkeletonLimbNode1->LclTranslation.Set(FbxVector4(0.0, 40.0, 0.0));
  // //  lSkeletonRoot->AddChild(lSkeletonLimbNode1);

  // for (auto i = 0; i < l_for_size; ++i) {  // 再写 骨骼

  //   // #ifdef DOODLE_1001
  //   auto* l_sk_attr = FbxSkeleton::Create(l_scene, fmt::format("skeleton_{}", i).c_str());
  //   l_sk_attr->SetSkeletonType(FbxSkeleton::eLimbNode);
  //   //    l_sk_attr->Size.Set(1.0);

  //   auto* l_bone_node = FbxNode::Create(l_scene, fmt::format("skeleton_{}", i).c_str());
  //   l_bone_node->SetNodeAttribute(l_sk_attr);
  //   l_bone_node->SetRotationOrder(FbxNode::eSourcePivot, eEulerXYZ);
  //   auto l_box = in_poly.getSchema().getSelfBoundsProperty().getValue().center();
  //   l_bone_node->LclTranslation.Set(FbxDouble3{l_box.x, l_box.y, l_box.z});
  //   l_bones.emplace_back(l_bone_node);
  //   l_scene->GetRootNode()->AddChild(l_bone_node);
  //   // #endif
  // }
  // //  return write_fbx_impl(l_scene, in_fbx_path);

  // {  // 写一个皮肤变形器
  //   auto* l_sk = FbxSkin::Create(l_scene, "");
  //   //    dynamic_cast<FbxGeometry*>(l_mesh_node->GetNodeAttribute())->AddDeformer(l_sk);
  //   //    FbxGeometry* lPatchAttribute = (FbxGeometry*)l_mesh_node->GetNodeAttribute();
  //   //    lPatchAttribute->AddDeformer(l_sk);

  //   for (int l = 0; l < l_for_size; ++l) {
  //     auto* l_cluster = FbxCluster::Create(l_scene, "");
  //     l_cluster->SetLink(l_bones[l]);
  //     l_cluster->SetLinkMode(FbxCluster::eTotalOne);
  //     l_cluster->AddControlPointIndex(l, 1.0);

  //     l_cluster->SetTransformMatrix(l_mesh_node->EvaluateGlobalTransform());
  //     l_cluster->SetTransformLinkMatrix(l_bones[l]->EvaluateGlobalTransform());
  //     if (!l_sk->AddCluster(l_cluster)) doodle::default_logger_raw()->info("add cluster error: {}", l);
  //   }
  //   l_mesh->AddDeformer(l_sk);
  //   doodle::default_logger_raw()->info("sk cluster size {}", l_sk->GetClusterCount());
  //   // 绑定post
  //   auto* l_post = FbxPose::Create(l_scene, "bind_pose");
  //   l_post->SetIsBindPose(true);
  //   for (auto* l_bone : l_bones) {
  //     if (l_post->Add(l_bone, l_bone->EvaluateGlobalTransform()) == -1) {
  //       doodle::default_logger_raw()->info("add bone error: {}", l_bone->GetName());
  //     }
  //   }
  //   l_post->Add(l_mesh_node, l_mesh_node->EvaluateGlobalTransform());

  //   l_scene->AddPose(l_post);
  // }

  std::vector<fbxsdk::FbxBlendShapeChannel*> l_blend_channels{};
  {  // 写混变
    auto* l_blend = fbxsdk::FbxBlendShape::Create(l_scene, "blend_shape");
    for (auto col = 0; col < in_poly.num_blend_shape_; ++col) {
      auto* l_shape = fbxsdk::FbxShape::Create(l_scene, fmt::format("shape_{}", col).c_str());
      l_shape->InitControlPoints(l_mesh->GetControlPointsCount());
      auto* l_blend_channel =
          fbxsdk::FbxBlendShapeChannel::Create(l_scene, fmt::format("blend_shape_channel_{}", col).c_str());

      auto* l_shape_pos = l_shape->GetControlPoints();
      // 顶点
      std::copy(l_mesh->GetControlPoints(), l_mesh->GetControlPoints() + l_mesh->GetControlPointsCount(), l_shape_pos);
      for (auto i = 0; i < l_mesh->GetControlPointsCount(); ++i) {
        auto l_index = i * 3;
        l_shape_pos[i] += fbxsdk::FbxVector4{
            (std::double_t)in_poly.bs_mesh_(l_index, col), (std::double_t)in_poly.bs_mesh_(l_index + 1, col),
            (std::double_t)in_poly.bs_mesh_(l_index + 2, col)
        };
      }
      l_blend_channel->AddTargetShape(l_shape);
      l_blend_channels.emplace_back(l_blend_channel);
      l_blend->AddBlendShapeChannel(l_blend_channel);
    }
    l_mesh->AddDeformer(l_blend);
  }

  {  // 写出动画曲线

    fbxsdk::FbxTime l_fbx_time{};
    l_fbx_time.SetFrame(1000, fbxsdk::FbxTime::ePAL);
    anim_stack->LocalStart  = l_fbx_time;
    l_fbx_time.SetFrame(1000 + in_poly.weight_.cols(), fbxsdk::FbxTime::ePAL);
    anim_stack->LocalStop   = l_fbx_time;
    Eigen::MatrixXd l_curve = in_poly.weight_.transpose();
    for (auto i = 0; i < in_poly.num_blend_shape_; ++i) {
      auto l_anim_curve = l_blend_channels[i]->DeformPercent.GetCurve(anim_layer, true);
      // std::cout << "curve " << i << "\n";
      for (auto j = 0; j < l_curve.cols(); ++j) {
        l_anim_curve->KeyModifyBegin();
        l_fbx_time.SetFrame(j + 1000, fbxsdk::FbxTime::ePAL);
        auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
        l_anim_curve->KeySet(l_key_index, l_fbx_time, l_curve(i, j) * 100);
        // std::cout << l_curve(i, j) << " ";
        l_anim_curve->KeyModifyEnd();
      }
      // std::cout << std::endl;
    }
  }

  return write_fbx_impl(l_scene, in_fbx_path);
}

Eigen::MatrixXd load_abc_mesh(
    const std::filesystem::path& in_path, std::vector<std::size_t>& in_face_size,
    std::vector<std::int64_t>& in_face_index
) {
  Alembic::Abc::IArchive l_ar{Alembic::AbcCoreOgawa::ReadArchive{}, in_path.generic_string()};
  auto l_top        = l_ar.getTop();
  const auto l_meta = l_top.getMetaData();

  Eigen::MatrixXd l_result;

  if (!Alembic::AbcGeom::IXform::matches(l_meta)) {
    doodle::default_logger_raw()->info("name {}", l_top.getName());
    const auto l_clild_count = l_top.getNumChildren();
    for (auto i = 0; i < l_clild_count; ++i) {
      auto l_child = l_top.getChild(i);
      // doodle::default_logger_raw()->info("name {}", l_child.getName());
      auto l_mesh  = l_child.getChild(0);
      if (Alembic::AbcGeom::IPolyMesh::matches(l_mesh.getMetaData())) {
        Alembic::AbcGeom::IPolyMesh l_poly{l_mesh};
        auto& l_s   = l_poly.getSchema();
        auto l_time = l_s.getTimeSampling();
        l_result.resize(l_s.getPositionsProperty().getValue()->size() * 3, l_s.getNumSamples());
        for (std::int64_t i = 0; i < l_s.getNumSamples(); ++i) {
          auto l_v = l_s.getPositionsProperty().getValue(Alembic::Abc::ISampleSelector{i});
          for (auto i2 = 0; i2 < l_v->size(); ++i2) {
            l_result(i2 * 3 + 0, i) = (*l_v)[i2].x;
            l_result(i2 * 3 + 1, i) = (*l_v)[i2].y;
            l_result(i2 * 3 + 2, i) = (*l_v)[i2].z;
          }
          // l_result.col(i) = Eigen::VectorXf{l_v->get(), l_v->size()};
        }
        doodle::default_logger_raw()->info("name {} size {}", l_child.getName(), l_result.rows() / 3);

        auto l_face_index = l_s.getValue().getFaceIndices();
        in_face_index.resize(l_face_index->size());
        for (auto i = 0; i < l_face_index->size(); ++i) {
          in_face_index[i] = (*l_face_index)[i];
        }
        auto l_face_size = l_s.getValue().getFaceCounts();
        in_face_size.resize(l_face_size->size());
        for (auto i = 0; i < l_face_size->size(); ++i) {
          in_face_size[i] = (*l_face_size)[i];
        }

        // std::cout << l_result << std::endl;
        return l_result;
      }
    }
  }
  return {};
}

BOOST_AUTO_TEST_CASE(blendshape_fbx) {
  std::vector<std::size_t> l_face_size{};
  std::vector<std::int64_t> l_face_index{};

  auto l_mesh =
      load_abc_mesh("E:/Doodle/src/test/core/DBXY_EP360_SC001_AN_TianMeng_rig_HL_.abc", l_face_size, l_face_index);

  Eigen::VectorXd Average = l_mesh.rowwise().mean();
  default_logger_raw()->info("mesh size {} {}", l_mesh.rows(), l_mesh.cols());
  // l_

  // std::cout << "Average: \n" << Average << std::endl;
  // std::cout << "mesh: \n" << l_mesh << std::endl;
  // 计算标准差
  Eigen::MatrixXd l_org  = l_mesh;
  Eigen::MatrixXd l_org2 = l_mesh;

  l_org.array().colwise() -= Average.array();
  // std::cout << "l_org: \n" << l_org << std::endl;

  // for (auto i = 0; i < l_org2.rows(); ++i) {
  //   l_org2.row(i).array() -= Average(i);
  // }
  // if (l_org.isApprox(l_org2)) {
  //   std::cout << "l_org == l_org2" << std::endl;
  // } else {
  //   std::cout << "l_org != l_org2" << std::endl;
  //   std::cout << "l_org2: \n" << l_org2 << std::endl;
  // }

  Eigen::JacobiSVD<Eigen::MatrixXd> l_svd{l_org, Eigen::ComputeThinU | Eigen::ComputeThinV};
  Eigen::MatrixXd l_u  = l_svd.matrixU();
  Eigen::MatrixXd l_v  = l_svd.matrixV();
  Eigen::VectorXd l_s  = l_svd.singularValues();

  // std::cout << "U: \n" << l_u << "\nV: \n" << l_v << "\nS: \n" << l_s << std::endl;

  Eigen::MatrixXd l_u2 = l_u;

  // 每行特征值乘以特征向量
  l_u.array().rowwise() *= l_s.transpose().array();
  // for (auto i = 0ll; i < l_s.size(); ++i) {
  //   const std::float_t l_muliplier = l_s(i);
  //   l_u2.col(i) *= l_muliplier;
  //   // for (auto j = 0ll; j < l_u.rows(); ++j) {
  //   //   l_u2(j, i) *= l_muliplier;
  //   // }
  // }
  // if (l_u2.isApprox(l_u)) {
  //   std::cout << "l_u2 == l_org" << std::endl;
  // } else {
  //   std::cout << "l_u2 != l_org" << std::endl;
  // }
  // std::cout << "mu u: \n" << l_u << std::endl;

  // std::cout << "l_v.transpose(): \n" << l_v.transpose() << std::endl;

  Eigen::VectorXd l_diff = l_u.cwiseAbs().colwise().sum();
  // std::cout << "diff: \n" << l_diff << std::endl;

  bl_mesh_data l_data;
  l_data.base_mesh_ = Average;
  l_data.bs_mesh_   = l_u;
  l_data.weight_    = l_v;
  for (auto i = 0; i < l_diff.size(); ++i) {
    if (l_diff(i) < 0.001) {
      l_data.num_blend_shape_ = i;
      break;
    }
  }
  l_data.num_blend_shape_ = l_u.cols();

  write_fbx("E:/Doodle/src/test/core/test_bl_end.fbx", l_data, l_face_size, l_face_index);
}