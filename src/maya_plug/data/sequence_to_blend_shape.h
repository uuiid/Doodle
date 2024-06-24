//
// Created by TD on 2022/8/15.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

#include <fbxsdk.h>
#include <maya/MFnMesh.h>
#include <Eigen/Eigen>

namespace doodle::maya_plug {
class fbx_write;
/**
 * @brief 将一系列网格变形动画转换为混合变形节点
 */
class sequence_to_blend_shape {
 private:
 
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
  void init(const MDagPath& in_mesh, std::int64_t in_samples);

 public:
  sequence_to_blend_shape();
  explicit sequence_to_blend_shape(const MDagPath& in_mesh, std::int64_t in_samples) : sequence_to_blend_shape() {
    init(in_mesh, in_samples);
  }

  virtual ~sequence_to_blend_shape();

  sequence_to_blend_shape(sequence_to_blend_shape&& in) noexcept;
  sequence_to_blend_shape& operator=(sequence_to_blend_shape&& in) noexcept;

  void add_sample(std::int64_t in_sample_index);

  // 计算
  void compute();

  // 写出为fbx
  void write_fbx(const fbx_write& in_node) const;
};

}  // namespace doodle::maya_plug
