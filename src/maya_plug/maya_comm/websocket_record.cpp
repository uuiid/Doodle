//
// Created by TD on 2026/9/4.
//

#include "websocket_record.h"

#include <maya_plug/node/websocket_client_node.h>
#include <maya_plug/node/websocket_record_node.h>

#include <boost/math/interpolators/makima.hpp>

#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

#include <algorithm>
#include <functional>
#include <numeric>
#include <vector>

namespace doodle::maya_plug {

MSyntax websocket_record_syntax() {
  MSyntax l_syntax{};
  l_syntax.addFlag("-n", "-node", MSyntax::kString);
  return l_syntax;
}

namespace {
MStatus get_client_node(const MArgList& in_list, websocket_client_node*& out_node) {
  MStatus l_status{};
  MArgDatabase l_arg_data{websocket_record_syntax(), in_list, &l_status};
  maya_chick(l_status);

  if (!l_arg_data.isFlagSet("-n")) {
    display_error("缺少 -n/-node 参数: 需要传入 websocket_client_node 节点名称");
    return MS::kInvalidParameter;
  }

  MString l_node_name{};
  maya_chick(l_arg_data.getFlagArgument("-n", 0, l_node_name));

  MSelectionList l_selection_list{};
  maya_chick(l_selection_list.add(l_node_name));
  MObject l_node{};
  maya_chick(l_selection_list.getDependNode(0, l_node));

  MFnDependencyNode l_fn_node{l_node, &l_status};
  maya_chick(l_status);
  if (l_fn_node.typeId() != websocket_client_node::doodle_id) {
    display_error("传入节点不是 doodle_websocket_client 节点: {}", conv::to_s(l_node_name));
    return MS::kInvalidParameter;
  }

  out_node = static_cast<websocket_client_node*>(l_fn_node.userNode());
  if (!out_node) return MS::kFailure;
  return MS::kSuccess;
}
}  // namespace

websocket_record_start::websocket_record_start()  = default;
websocket_record_start::~websocket_record_start() = default;

MStatus websocket_record_start::doIt(const MArgList& in_list) {
  websocket_client_node* l_node{};
  maya_chick(get_client_node(in_list, l_node));
  l_node->start_record();
  display_info("已开始录制 websocket_client_node 接收的数据");
  return MS::kSuccess;
}

websocket_record_stop::websocket_record_stop()  = default;
websocket_record_stop::~websocket_record_stop() = default;

std::vector<double> websocket_record_stop::resample_to_frame_rate(
    const std::vector<std::string>& in_names, const std::vector<double>& in_values
) {
  const auto l_channel_count = in_names.size();
  if (l_channel_count == 0 || in_values.empty()) return in_values;

  const auto l_stride = l_channel_count + 1;  // 每帧 [时间, 通道值...]
  if (in_values.size() % l_stride != 0) return in_values;

  const auto l_frame_count = in_values.size() / l_stride;
  if (l_frame_count < 2) return in_values;

  // 按时间排序帧索引
  std::vector<std::size_t> l_order(l_frame_count);
  std::iota(l_order.begin(), l_order.end(), std::size_t{0});
  std::sort(l_order.begin(), l_order.end(), [&](std::size_t a, std::size_t b) {
    return in_values[a * l_stride] < in_values[b * l_stride];
  });

  // 按排序后顺序拆分时间与各通道
  std::vector<double> l_times(l_frame_count);
  std::vector<std::vector<double>> l_channels(l_channel_count);
  for (auto& c : l_channels) c.resize(l_frame_count);
  for (std::size_t i = 0; i < l_frame_count; ++i) {
    const auto l_src = l_order[i] * l_stride;
    l_times[i]       = in_values[l_src];
    for (std::size_t j = 0; j < l_channel_count; ++j)
      l_channels[j][i] = in_values[l_src + 1 + j];
  }

  // makima 三次插值至少需要 4 个数据点
  if (l_frame_count < 4) return in_values;

  // 目标时间网格 (当前帧率)
  const auto l_fps = details::fps();
  if (l_fps <= 0.0) return in_values;
  const auto l_spf   = 1.0 / l_fps;
  const auto l_start = l_times.front();
  const auto l_end   = l_times.back();

  std::vector<double> l_target_times{};
  for (auto t = l_start; t <= l_end + 1e-9; t += l_spf) l_target_times.push_back(t);
  if (l_target_times.empty()) l_target_times.push_back(l_start);

  // 每通道构建 makima 三次插值
  std::vector<std::function<double(double)>> l_interps{};
  l_interps.reserve(l_channel_count);
  for (std::size_t j = 0; j < l_channel_count; ++j) {
    boost::math::interpolators::makima<std::vector<double>> l_spline{
        std::vector<double>{l_times}, std::vector<double>{l_channels[j]}};
    l_interps.emplace_back([l_spline = std::move(l_spline)](double t) { return l_spline(t); });
  }

  std::vector<double> l_result{};
  l_result.reserve(l_target_times.size() * l_stride);
  for (std::size_t k = 0; k < l_target_times.size(); ++k) {
    l_result.push_back(l_target_times[k]);
    for (auto& l_f : l_interps) l_result.push_back(l_f(l_target_times[k]));
  }
  return l_result;
}

MStatus websocket_record_stop::doIt(const MArgList& in_list) {
  websocket_client_node* l_node{};
  maya_chick(get_client_node(in_list, l_node));

  auto [l_names, l_values] = l_node->stop_record();
  l_values                 = resample_to_frame_rate(l_names, l_values);

  MStatus l_status{};
  MFnDependencyNode l_fn_node{};
  MObject l_record_obj = l_fn_node.create(websocket_record_node::doodle_id, &l_status);
  maya_chick(l_status);

  // 写入属性名列表
  {
    MPlug l_plug = l_fn_node.findPlug(websocket_record_node::output_names, true, &l_status);
    maya_chick(l_status);
    maya_chick(l_plug.setNumElements(static_cast<unsigned int>(l_names.size())));
    for (auto i = 0; i < l_names.size(); ++i) {
      auto l_elem = l_plug.elementByLogicalIndex(static_cast<unsigned int>(i), &l_status);
      maya_chick(l_status);
      maya_chick(l_elem.setValue(conv::to_ms(l_names[i])));
    }
  }
  // 写入属性值列表
  {
    MPlug l_plug = l_fn_node.findPlug(websocket_record_node::output_values, true, &l_status);
    maya_chick(l_status);
    maya_chick(l_plug.setNumElements(static_cast<unsigned int>(l_values.size())));
    for (auto i = 0; i < l_values.size(); ++i) {
      auto l_elem = l_plug.elementByLogicalIndex(static_cast<unsigned int>(i), &l_status);
      maya_chick(l_status);
      maya_chick(l_elem.setValue(l_values[i]));
    }
  }

  setResult(MString{l_fn_node.name().asChar()});
  display_info("已结束录制并创建节点保存数据: 名称 {} 个, 数值 {} 个", l_names.size(), l_values.size());
  return MS::kSuccess;
}

}  // namespace doodle::maya_plug
