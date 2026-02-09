//
// Created by TD on 2024/1/11.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <maya/MPxNode.h>
namespace doodle::maya_plug {
class dna_calib_node : public MPxNode {
 public:
  dna_calib_node();
  ~dna_calib_node() override;
  static MTypeId doodle_id;
  const static constexpr auto node_type = MPxNode::Type::kDependNode;
  static void* creator();
  static MStatus initialize();

  const static constexpr std::string_view node_name{"doodle_dna_calib_node"};

  // DNA文件路径
  static MObject dna_file_path;
  // 控制Gui输入
  static MObject gui_control_list;
  // 骨骼位移输出
  static MObject output_join_transforms;
  // 骨骼旋转输出
  static MObject output_join_rotations;
  // 骨骼缩放输出
  static MObject output_join_scales;
  // 混合变形输出
  static MObject output_blendshape_weights;

  class impl;
  std::unique_ptr<impl> p_i;

 private:
};
}  // namespace doodle::maya_plug