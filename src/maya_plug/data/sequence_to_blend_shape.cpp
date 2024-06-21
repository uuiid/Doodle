//
// Created by TD on 2022/8/15.
//

#include "sequence_to_blend_shape.h"

#include "doodle_core/exception/exception.h"

#include "main/maya_plug_fwd.h"
#include <maya_plug/data/fbx_write.h>
#include <maya_plug/data/maya_file_io.h>
#include <maya_plug/data/reference_file.h>

#include <Eigen/Eigen>
#include <maya/MAnimControl.h>
#include <maya/MArgDatabase.h>
#include <maya/MBoundingBox.h>
#include <maya/MComputation.h>
#include <maya/MDGContextGuard.h>
#include <maya/MDagModifier.h>
#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDataHandle.h>
#include <maya/MDoubleArray.h>
#include <maya/MEulerRotation.h>
#include <maya/MFloatPointArray.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnIkJoint.h>
#include <maya/MFnMesh.h>
#include <maya/MFnSet.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MItSelectionList.h>
#include <maya/MMatrix.h>
#include <maya/MMatrixArray.h>
#include <maya/MNamespace.h>
#include <maya/MPointArray.h>
#include <maya/MQuaternion.h>
#include <maya/MSelectionList.h>
#include <maya/MStatus.h>
#include <maya/MTime.h>
#include <maya/MTransformationMatrix.h>
// #define DOODLE_USE_SELECT_MODEL_COPY_AS_BIND_MODEL

namespace doodle::maya_plug {

class sequence_to_blend_shape::impl {
 public:
  // 需要计算的动画数据
  Eigen::MatrixXd anim_mesh_{};

  // 混合变形中的基本形状
  Eigen::VectorXd base_mesh_{};
  std::vector<std::vector<Eigen::Vector3d>> normal_{};

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
  std::vector<Eigen::Vector3d> mesh_off_{};

  MDagPath base_mesh_obj_{};
};

sequence_to_blend_shape::sequence_to_blend_shape() : ptr(std::make_unique<impl>()) {}

void sequence_to_blend_shape::init(const MDagPath& in_mesh, std::int64_t in_samples) {
  ptr->base_mesh_obj_ = in_mesh;

  MStatus l_status{};
  MFnMesh l_mesh{};
  l_status = l_mesh.setObject(in_mesh);
  maya_chick(l_status);
  const auto l_num_points = l_mesh.numVertices();
  ptr->anim_mesh_.resize(l_num_points * 3, in_samples);
  const auto l_num_poly = l_mesh.numPolygons();
  ptr->normal_.resize(l_num_poly);
}

void sequence_to_blend_shape::add_sample(std::int64_t in_sample_index) {
  MFnMesh l_fn_mesh{ptr->base_mesh_obj_};

  MPointArray l_m_points{};
  l_fn_mesh.getPoints(l_m_points, MSpace::kObject);

  if (l_m_points.length() != ptr->anim_mesh_.rows() / 3) {
    throw_exception(doodle_error{"动画数据点数与基础形状点数不一致"});
  }
  if (in_sample_index >= ptr->anim_mesh_.cols()) {
    throw_exception(doodle_error{"采样数超出范围"});
  }

  for (std::size_t i = 0; i < l_m_points.length(); ++i) {
    ptr->anim_mesh_(i * 3 + 0, in_sample_index) = l_m_points[i].x;
    ptr->anim_mesh_(i * 3 + 1, in_sample_index) = l_m_points[i].y;
    ptr->anim_mesh_(i * 3 + 2, in_sample_index) = l_m_points[i].z;
  }

  // 添加法线
  for (auto i = 0; i < l_fn_mesh.numPolygons(); ++i) {
    MIntArray l_vert_list{};
    maya_chick(l_fn_mesh.getPolygonVertices(i, l_vert_list));
    auto& l_n = ptr->normal_[i];
    if (l_n.empty()) l_n.resize(l_vert_list.length());

    for (auto j = 0; j < l_vert_list.length(); ++j) {
      MVector l_normal{};
      maya_chick(l_fn_mesh.getFaceVertexNormal(i, l_vert_list[j], l_normal, MSpace::kObject));
      l_n[j] += Eigen::Vector3d{l_normal.x, l_normal.y, l_normal.z};
    }
  }
}

void sequence_to_blend_shape::compute() {
  // 计算法线
  for (auto& l_n : ptr->normal_) {
    for (auto& l_v : l_n) {
      l_v.normalize();
    }
  }

  ptr->mesh_off_.resize(ptr->anim_mesh_.cols());

  for (auto i = 0; i < ptr->anim_mesh_.cols(); ++i) {
    Eigen::AlignedBox3d l_box{};
    for (auto j = 0; j < ptr->anim_mesh_.rows() / 3; ++j) {
      l_box.extend(Eigen::Vector3d{ptr->anim_mesh_.block<3, 1>(j * 3, i)});
    }
    Eigen::Vector3d l_off = l_box.center();
    ptr->mesh_off_.emplace_back(l_off);
    for (auto j = 0; j < ptr->anim_mesh_.rows() / 3; ++j) {
      ptr->anim_mesh_.block<3, 1>(j * 3, i) -= l_off;
    }
  }
  // 平均
  ptr->base_mesh_ = ptr->anim_mesh_.rowwise().mean();
  // 计算标准差
  ptr->anim_mesh_.array().colwise() -= ptr->base_mesh_.array();

  // svd分解
  Eigen::JacobiSVD<Eigen::MatrixXd> l_svd{ptr->anim_mesh_, Eigen::ComputeThinU | Eigen::ComputeThinV};
  ptr->bs_mesh_ = l_svd.matrixU();
  ptr->weight_  = l_svd.matrixV().transpose();

  // 乘以奇异值
  ptr->bs_mesh_.array().colwise() *= l_svd.singularValues().array();

  // 计算优化
  Eigen::VectorXd l_diff = ptr->bs_mesh_.cwiseAbs().colwise().sum();
  ptr->num_blend_shape_  = l_diff.size();

  for (auto i = 0; i < l_diff.size(); ++i) {
    if (l_diff[i] < 0.01) {
      ptr->num_blend_shape_ = i;
      break;
    }
  }
  // 完成计算
}

void sequence_to_blend_shape::write_fbx(const fbx_write& in_node) const {
  auto l_my_node = in_node.find_node(ptr->base_mesh_obj_);

  if (auto l_mesh = dynamic_cast<fbx_write_ns::fbx_node_mesh*>(l_my_node); l_mesh != nullptr) {
    l_mesh->build_mesh();
  } else {
    throw_exception(doodle_error{"不是网格节点"});
  }

  auto l_node             = l_my_node->node;
  auto&& [l_begin, l_end] = in_node.get_anim_time();

  auto* l_node_attr       = l_node->GetNodeAttribute();
  if (l_node_attr->GetAttributeType() != fbxsdk::FbxNodeAttribute::eMesh) {
    throw_exception(doodle_error{"不是网格节点"});
  }

  auto l_mesh = static_cast<fbxsdk::FbxMesh*>(l_node_attr);

  MFnMesh l_fn_mesh{ptr->base_mesh_obj_};

  {  // 调整顶点
    // l_mesh->InitControlPoints(ptr->base_mesh_.size() / 3);
    auto* l_points = l_mesh->GetControlPoints();

    for (auto i = 0; i < ptr->base_mesh_.size() / 3; ++i) {
      l_points[i] =
          fbxsdk::FbxVector4{ptr->base_mesh_[i * 3 + 0], ptr->base_mesh_[i * 3 + 1], ptr->base_mesh_[i * 3 + 2]};
    }
    // 调整法线
    auto l_layer = l_mesh->GetElementNormal();
    for (auto i = 0, l_fbx_i = 0; i < l_fn_mesh.numPolygons(); ++i) {
      MIntArray l_vert_list{};
      maya_chick(l_fn_mesh.getPolygonVertices(i, l_vert_list));
      for (auto j = 0; j < l_vert_list.length(); ++j, ++l_fbx_i) {
        l_layer->GetDirectArray()[l_fbx_i] =
            fbxsdk::FbxVector4{ptr->normal_[i][j].x(), ptr->normal_[i][j].y(), ptr->normal_[i][j].z()};
      }
    }
  }

  std::vector<fbxsdk::FbxBlendShapeChannel*> l_blend_channels{};
  {  // 添加混合变形
    auto l_blend_shape =
        fbxsdk::FbxBlendShape::Create(l_node->GetScene(), fmt::format("{}_blend_shape", l_node->GetName()).c_str());

    for (auto i = 0; i < ptr->num_blend_shape_; ++i) {
      auto* l_shape = fbxsdk::FbxShape::Create(l_node->GetScene(), fmt::format("shape_{}", i).c_str());
      l_shape->InitControlPoints(l_mesh->GetControlPointsCount());
      auto* l_blend_channel =
          fbxsdk::FbxBlendShapeChannel::Create(l_node->GetScene(), fmt::format("blend_shape_channel_{}", i).c_str());

      auto* l_shape_pos = l_shape->GetControlPoints();
      // 顶点
      std::copy(l_mesh->GetControlPoints(), l_mesh->GetControlPoints() + l_mesh->GetControlPointsCount(), l_shape_pos);
      for (auto i = 0; i < l_mesh->GetControlPointsCount(); ++i) {
        auto l_index = i * 3;
        l_shape_pos[i] += fbxsdk::FbxVector4{
            ptr->bs_mesh_(l_index + 0, i), ptr->bs_mesh_(l_index + 1, i), ptr->bs_mesh_(l_index + 2, i)
        };
      }
      l_blend_channel->AddTargetShape(l_shape);
      l_blend_channels.emplace_back(l_blend_channel);
      l_blend_shape->AddBlendShapeChannel(l_blend_channel);
    }
    l_mesh->AddDeformer(l_blend_shape);
  }

  {  // 写出动画曲线

    fbxsdk::FbxTime l_fbx_time{};

    Eigen::MatrixXd l_curve = ptr->weight_.transpose();
    // Eigen::MatrixXd l_curve = ptr->weight_;
    auto* l_layer           = l_node->GetScene()->GetCurrentAnimationStack()->GetMember<FbxAnimLayer>();
    for (auto i = 0; i < ptr->num_blend_shape_; ++i) {
      auto l_anim_curve = l_blend_channels[i]->DeformPercent.GetCurve(l_layer, true);
      // std::cout << "curve " << i << "\n";
      l_anim_curve->KeyModifyBegin();
      for (auto j = 0; j < l_curve.cols(); ++j) {
        l_fbx_time.SetFrame(l_begin.value() + j, fbx_write_ns::fbx_node::maya_to_fbx_time(l_begin.unit()));
        auto l_key_index = l_anim_curve->KeyAdd(l_fbx_time);
        l_anim_curve->KeySet(l_key_index, l_fbx_time, l_curve(i, j) * 100);
        // std::cout << l_curve(i, j) << " ";
      }
      l_anim_curve->KeyModifyEnd();
    }

    // 写出偏移曲线
    auto l_c_x = l_node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_X, true);
    auto l_c_y = l_node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Y, true);
    auto l_c_z = l_node->LclTranslation.GetCurve(l_layer, FBXSDK_CURVENODE_COMPONENT_Z, true);
    for (auto j = 0; j < l_curve.cols(); ++j) {
      l_fbx_time.SetFrame(l_begin.value() + j, fbx_write_ns::fbx_node::maya_to_fbx_time(l_begin.unit()));

      l_c_x->KeyModifyBegin();
      auto l_key_index = l_c_x->KeyAdd(l_fbx_time);
      l_c_x->KeySet(l_key_index, l_fbx_time, ptr->mesh_off_[j].x());
      l_c_x->KeyModifyEnd();

      l_c_y->KeyModifyBegin();
      l_key_index = l_c_y->KeyAdd(l_fbx_time);
      l_c_y->KeySet(l_key_index, l_fbx_time, ptr->mesh_off_[j].y());
      l_c_y->KeyModifyEnd();

      l_c_z->KeyModifyBegin();
      l_key_index = l_c_z->KeyAdd(l_fbx_time);
      l_c_z->KeySet(l_key_index, l_fbx_time, ptr->mesh_off_[j].z());
      l_c_z->KeyModifyEnd();
    }
  }
}

sequence_to_blend_shape::~sequence_to_blend_shape() = default;
}  // namespace doodle::maya_plug
