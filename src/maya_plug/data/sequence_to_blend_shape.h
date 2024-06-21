//
// Created by TD on 2022/8/15.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

#include <fbxsdk.h>
#include <maya/MFnMesh.h>
namespace doodle::maya_plug {

/**
 * @brief 将一系列网格变形动画转换为混合变形节点
 */
class sequence_to_blend_shape {
 private:
  class impl;
  std::unique_ptr<impl> ptr;

  void init(const MObject& in_mesh, std::int64_t in_samples);

 public:
  sequence_to_blend_shape();
  explicit sequence_to_blend_shape(const MObject& in_mesh, std::int64_t in_samples) : sequence_to_blend_shape() {
    init(in_mesh, in_samples);
  }

  virtual ~sequence_to_blend_shape();

  sequence_to_blend_shape(sequence_to_blend_shape&& in) noexcept;
  sequence_to_blend_shape& operator=(sequence_to_blend_shape&& in) noexcept;

  void add_sample(std::int64_t in_sample_index);

  // 计算
  void compute();

  // 写出为fbx
  void write_fbx(fbxsdk::FbxNode* in_node) const;
};

}  // namespace doodle::maya_plug
