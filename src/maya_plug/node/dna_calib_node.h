//
// Created by TD on 2024/1/11.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <maya/MPxNode.h>
namespace doodle::maya_plug {
class dna_calib_node : public MPxNode {
 public:
  static MTypeId doodle_id;
  const static constexpr auto node_type = MPxNode::Type::kDependNode;
  static void* creator();
  static MStatus initialize();

  const static constexpr std::string_view node_name{"doodle_dna_calib_node"};

 private:
  // DNA文件路径
  static MObject dna_file_path;
  // 校准结果输出路径
  static MObject output_file_path;
  // 校准结果输出路径
  static MObject output_image_path;

  // 起始帧
  static MObject start_frame;
  // 结束帧
  static MObject end_frame;
  // 步长
  static MObject frame_step;

  // 校准结果
  static MObject calib_result;
};
}  // namespace doodle::maya_plug