//
// Created by TD on 2026/9/2.
//

#include "websocket_client_node.h"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>

#include "data/maya_display.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <data/maya_conv_str.h>
#include <maya/MArrayDataBuilder.h>
#include <maya/MArrayDataHandle.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <memory>
#include <msgpack.hpp>
#include <string>
#include <thread>
#include <vector>

namespace doodle::maya_plug {
namespace beast     = boost::beast;
namespace websocket = beast::websocket;
namespace net       = boost::asio;
using tcp           = net::ip::tcp;

MObject websocket_client_node::ip_address{};
MObject websocket_client_node::port{};
MObject websocket_client_node::output_names{};
MObject websocket_client_node::output_values{};

MTypeId websocket_client_node::doodle_id = MTypeId{0x00000003};

class websocket_client_node::impl_t {
 public:
  impl_t()  = default;
  ~impl_t() = default;

  std::string ip_{"127.0.0.1"};
  int port_{8890};
};

namespace {
// 缓冲区负载类型
enum class buffer_type : std::int32_t { k_names = 0, k_values = 1 };

// 解析后的负载数据, 通过裸指针在缓冲区中传递, 避免二次拷贝
struct payload {
  buffer_type type{};
  std::vector<std::string> names{};
  double time{};
  std::vector<double> values{};
};

// 解析一条 WebSocket 消息, 判断是属性名还是属性值, 并填充到负载中
bool fill_payload(payload& in_payload, const std::string& in_msg) {
  try {
    msgpack::object_handle l_handle = msgpack::unpack(in_msg.data(), in_msg.size());
    msgpack::object l_obj           = l_handle.get();
    if (l_obj.type != msgpack::type::ARRAY || l_obj.via.array.size == 0) return false;

    if (l_obj.via.array.ptr[0].type == msgpack::type::STR) {
      in_payload.type = buffer_type::k_names;
      l_obj.convert(in_payload.names);
    } else {
      in_payload.type = buffer_type::k_values;
      std::vector<double> l_values{};
      l_obj.convert(l_values);
      // 首项为时间, 其余为通道值
      if (!l_values.empty()) {
        in_payload.time = l_values.front();
        in_payload.values.assign(l_values.begin() + 1, l_values.end());
      }
    }
    return true;
  } catch (const std::exception& e) {
    display_warning("解析 WebSocket 消息失败: {}", e.what());
    return false;
  }
}
}  // namespace

websocket_client_node::websocket_client_node() : p_i(std::make_unique<impl_t>()) {}
websocket_client_node::~websocket_client_node() { destroyMemoryPools(); }

void* websocket_client_node::creator() { return new websocket_client_node{}; }
websocket_client_node::impl_t* websocket_client_node::impl() { return p_i.get(); }

MStatus websocket_client_node::initialize() {
  MStatus l_status{};

  {  // WebSocket 服务器 ip 地址
    MFnTypedAttribute l_typed_attr{};
    ip_address = l_typed_attr.create("ip_address", "ip", MFnData::kString, MObject::kNullObj, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(ip_address));
  }
  {  // WebSocket 服务器端口
    MFnNumericAttribute l_numeric_attr{};
    port = l_numeric_attr.create("port", "port", MFnNumericData::kInt, 8890, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(port));
  }
  {  // 输出属性名列表
    MFnTypedAttribute l_typed_attr{};
    output_names = l_typed_attr.create("output_names", "out_n", MFnData::kString, MObject::kNullObj, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setUsesArrayDataBuilder(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_names));
  }
  {  // 输出属性值列表
    MFnNumericAttribute l_numeric_attr{};
    output_values = l_numeric_attr.create("output_values", "out_v", MFnNumericData::kDouble, 0, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(false));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setArray(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setUsesArrayDataBuilder(true));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output_values));
  }

  // // 基类设备节点属性 (serverName, deviceName, output, live, frameRate)
  // {
  //   MFnTypedAttribute l_typed_attr{};
  //   serverName = l_typed_attr.create("serverName", "srv", MFnData::kString, MObject::kNullObj, &l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(false));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(serverName));
  // }
  // {
  //   MFnTypedAttribute l_typed_attr{};
  //   deviceName = l_typed_attr.create("deviceName", "dev", MFnData::kString, MObject::kNullObj, &l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(false));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(deviceName));
  // }
  // {
  //   MFnTypedAttribute l_typed_attr{};
  //   output = l_typed_attr.create("output", "out", MFnData::kString, MObject::kNullObj, &l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setStorable(false));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setWritable(false));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_typed_attr.setReadable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(output));
  // }
  // {
  //   MFnNumericAttribute l_numeric_attr{};
  //   live = l_numeric_attr.create("live", "live", MFnNumericData::kBoolean, 1.0, &l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(live));
  // }
  // {
  //   MFnNumericAttribute l_numeric_attr{};
  //   frameRate = l_numeric_attr.create("frameRate", "fps", MFnNumericData::kDouble, 24.0, &l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setStorable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setWritable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_numeric_attr.setReadable(true));
  //   DOODLE_CHECK_MSTATUS_AND_RETURN_IT(addAttribute(frameRate));
  // }

  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(ip_address, output_values));
  DOODLE_CHECK_MSTATUS_AND_RETURN_IT(attributeAffects(port, output_values));
  return l_status;
}

void websocket_client_node::postConstructor() {
  // 在主线程中读取 ip 与端口, 供工作线程使用
  MFnDependencyNode l_fn_node{thisMObject()};
  MStatus l_status{};
  auto l_ip_plug = l_fn_node.findPlug(ip_address, false, &l_status);
  if (l_status) impl()->ip_ = conv::to_s(l_ip_plug.asString());
  auto l_port_plug = l_fn_node.findPlug(port, false, &l_status);
  if (l_status) impl()->port_ = l_port_plug.asInt();

  // 刷新输出属性
  MObjectArray l_attribute_list{};
  l_attribute_list.append(output_values);
  setRefreshOutputAttributes(l_attribute_list);

  // 创建内存池: 1024 个缓冲区, 每个缓冲区仅存放一个指针
  createMemoryPools(1024, 1, sizeof(void*));
}

void websocket_client_node::threadHandler() {
  const auto l_ip   = impl()->ip_;
  const auto l_port = impl()->port_;

  while (!isDone()) {
    try {
      net::io_context l_ioc{};
      websocket::stream<tcp::socket> l_ws{l_ioc};
      tcp::resolver l_resolver{l_ioc};

      auto const l_results = l_resolver.resolve(l_ip, std::to_string(l_port));
      net::connect(l_ws.next_layer(), l_results.begin(), l_results.end());

      // 设置超时, 使阻塞读可被 isDone 打断
      websocket::stream_base::timeout l_timeout{};
      l_timeout.idle_timeout     = std::chrono::milliseconds{500};
      l_timeout.keep_alive_pings = false;
      l_ws.set_option(l_timeout);

      l_ws.handshake(l_ip, "/");
      // 发送一个消息以请求服务器回传属性名
      l_ws.write(net::buffer(std::string{"hello"}));

      beast::flat_buffer l_buffer{};
      while (!isDone()) {
        beast::error_code l_ec{};
        l_ws.read(l_buffer, l_ec);
        if (l_ec) {
          if (l_ec == beast::error::timeout) {
            l_buffer.consume(l_buffer.size());
            continue;  // 空闲超时, 继续检查 isDone
          }
          break;  // 连接关闭或出错, 退出内层循环重连
        }

        const auto l_msg = beast::buffers_to_string(l_buffer.data());
        beginThreadLoop();
        MCharBuffer l_storage{};
        if (acquireDataStorage(l_storage) == MS::kSuccess) {
          auto l_payload = std::make_unique<payload>();
          if (fill_payload(*l_payload, l_msg)) {
            // 缓冲区中只存放 payload 裸指针
            auto l_raw_payload = l_payload.release();
            std::memcpy(l_storage.ptr(), &l_raw_payload, sizeof(l_raw_payload));
            pushThreadData(l_storage);
          } else {
            releaseDataStorage(l_storage);
          }
        }
        endThreadLoop();

        l_buffer.consume(l_buffer.size());
      }
    } catch (const std::exception& e) {
      display_warning("WebSocket 连接异常: {}", e.what());
    }

    // 断线后短暂等待再重连, 避免空转
    for (auto i = 0; i < 10 && !isDone(); ++i) std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
}

void websocket_client_node::threadShutdownHandler() { setDone(true); }

MStatus websocket_client_node::compute(const MPlug& in_plug, MDataBlock& in_data_block) {
  if (in_plug != output_values) return MS::kUnknownParameter;

  MCharBuffer l_storage{};
  if (popThreadData(l_storage) != MS::kSuccess) return MS::kSuccess;

  payload* l_payload{};
  std::memcpy(&l_payload, l_storage.ptr(), sizeof(l_payload));
  releaseDataStorage(l_storage);

  if (!l_payload) return MS::kSuccess;
  std::unique_ptr<payload> l_guard{l_payload};

  MStatus l_status{};
  if (l_guard->type == buffer_type::k_names) {
    const auto& l_names = l_guard->names;
    auto l_handle       = in_data_block.outputArrayValue(output_names, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_builder = l_handle.builder(&l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_builder.growArray(static_cast<unsigned int>(l_names.size())));
    for (auto i = 0; i < l_names.size(); ++i) {
      auto l_elem = l_builder.addElement(static_cast<unsigned int>(i), &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      l_elem.set(conv::to_ms(l_names[i]));
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_handle.set(l_builder));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_handle.setAllClean());
  } else if (l_guard->type == buffer_type::k_values) {
    const auto& l_values = l_guard->values;
    auto l_handle        = in_data_block.outputArrayValue(output_values, &l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    auto l_builder = l_handle.builder(&l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_builder.growArray(static_cast<unsigned int>(l_values.size())));
    for (auto i = 0; i < l_values.size(); ++i) {
      auto l_elem = l_builder.addElement(static_cast<unsigned int>(i), &l_status);
      DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_status);
      l_elem.set(l_values[i]);
    }
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_handle.set(l_builder));
    DOODLE_CHECK_MSTATUS_AND_RETURN_IT(l_handle.setAllClean());
  }

  return MS::kSuccess;
}

}  // namespace doodle::maya_plug
