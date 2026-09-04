//
// Created by TD on 2026/9/4.
//

#include "websocket_record.h"

#include <maya_plug/node/websocket_client_node.h>
#include <maya_plug/node/websocket_record_node.h>

#include <maya/MArgDatabase.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MPlug.h>
#include <maya/MString.h>
#include <maya/MSyntax.h>

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

MStatus websocket_record_stop::doIt(const MArgList& in_list) {
  websocket_client_node* l_node{};
  maya_chick(get_client_node(in_list, l_node));

  auto [l_names, l_values] = l_node->stop_record();

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
