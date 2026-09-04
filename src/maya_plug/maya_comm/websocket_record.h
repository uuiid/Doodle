//
// Created by TD on 2026/9/4.
//

#pragma once

#include <maya_plug/main/maya_plug_fwd.h>

#include <maya/MSelectionList.h>

#include <string>
#include <vector>

namespace doodle::maya_plug {

namespace websocket_record_start_ns {
constexpr char name[]{"doodle_websocket_record_start"};
}  // namespace websocket_record_start_ns

namespace websocket_record_stop_ns {
constexpr char name[]{"doodle_websocket_record_stop"};
}  // namespace websocket_record_stop_ns

MSyntax websocket_record_syntax();

/// @brief 开始录制命令: 传入节点名称, 让 websocket_client_node 开始记录接收到的数据
class websocket_record_start
    : public TemplateAction<websocket_record_start, websocket_record_start_ns::name, websocket_record_syntax> {
 public:
  websocket_record_start();
  ~websocket_record_start() override;
  MStatus doIt(const MArgList& in_list) override;
};

/// @brief 结束录制命令: 传入节点名称, 停止录制并创建一个 websocket_record_node 保存数据
class websocket_record_stop
    : public TemplateAction<websocket_record_stop, websocket_record_stop_ns::name, websocket_record_syntax> {
 public:
  websocket_record_stop();
  ~websocket_record_stop() override;
  MStatus doIt(const MArgList& in_list) override;

 private:
  // 使用 Boost.Math 将记录的数据插值重采样到当前帧率
  static std::vector<double> resample_to_frame_rate(
      const std::vector<std::string>& in_names, const std::vector<double>& in_values
  );
};

}  // namespace doodle::maya_plug
