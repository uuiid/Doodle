//
// Created by TD on 2026/9/2.
//

#pragma once

#include <maya_plug/maya_plug_fwd.h>

#include <maya/MPxClientDeviceNode.h>
#include <maya/MStatus.h>

#include <string>
#include <tuple>
#include <vector>
namespace doodle::maya_plug {
/// @brief 接收 WebSocket 数据并传递给 Maya 的客户端设备节点
class websocket_client_node : public MPxClientDeviceNode {
 public:
  websocket_client_node();
  ~websocket_client_node() override;

  static MTypeId doodle_id;
  const static constexpr auto node_type = MPxNode::Type::kClientDeviceNode;
  static void* creator();
  static MStatus initialize();

  const static constexpr std::string_view node_name{"doodle_websocket_client"};

  // 输出属性名列表
  static MObject output_names;
  // 输出属性值列表
  static MObject output_values;

  void postConstructor() override;
  MStatus compute(const MPlug& in_plug, MDataBlock& in_data_block) override;
  void threadHandler(const char* serverName, const char* deviceName) override;
  void threadShutdownHandler() override;

  // 开始录制: 清空录制缓冲并开始记录接收到的数据
  void start_record();
  // 结束录制: 返回 (属性名列表, 属性值列表[每帧为 时间 + 通道值])
  std::tuple<std::vector<std::string>, std::vector<double>> stop_record();

  class impl_t;
  impl_t* impl();

 private:
  std::unique_ptr<impl_t> p_i;
};
}  // namespace doodle::maya_plug
