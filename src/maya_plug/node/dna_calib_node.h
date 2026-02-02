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

  // DNA文件路径
  static MObject dna_file_path;
 private:
};
}  // namespace doodle::maya_plug