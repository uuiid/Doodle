//
// Created by TD on 2022/8/15.
//

#include "sequence_to_blend_shape.h"

#include "doodle_core/exception/exception.h"

#include "main/maya_plug_fwd.h"
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

  MObject base_mesh_obj_{};
};

sequence_to_blend_shape::sequence_to_blend_shape() : ptr(std::make_unique<impl>()) {}

void sequence_to_blend_shape::init(const MObject& in_mesh, std::int64_t in_samples) {
  ptr->base_mesh_obj_ = in_mesh;

  MStatus l_status{};
  MFnMesh l_mesh{};
  l_status = l_mesh.setObject(in_mesh);
  maya_chick(l_status);
  const auto l_num_points = l_mesh.numVertices();
  ptr->anim_mesh_.resize(l_num_points * 3, in_samples);
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
}

void sequence_to_blend_shape::compute() {
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

void sequence_to_blend_shape::write_fbx(fbxsdk::FbxNode* in_node) const {}

sequence_to_blend_shape::~sequence_to_blend_shape() = default;
}  // namespace doodle::maya_plug
