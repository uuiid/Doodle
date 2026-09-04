//
// Created by TD on 2026/9/4.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <maya/MPxNode.h>
#include <maya/MStatus.h>
namespace doodle::maya_plug {
/// @brief 保存录制的 WebSocket 数据的节点
class websocket_record_node : public MPxNode {
 public:
  websocket_record_node();
  ~websocket_record_node() override;

  static MTypeId doodle_id;
  const static constexpr auto node_type = MPxNode::Type::kDependNode;
  static void* creator();
  static MStatus initialize();

  const static constexpr std::string_view node_name{"doodle_websocket_record"};

  // 属性名列表
  static MObject output_names;
  // 属性值列表 (每帧为 [时间, 通道值...])
  static MObject output_values;

  MStatus compute(const MPlug& in_plug, MDataBlock& in_data_block) override;
};
}  // namespace doodle::maya_plug
