//
// Created by TD on 25-2-12.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <boost/signals2/signal.hpp>

namespace doodle::socket_io {
class sid_ctx;
struct socket_io_packet;
using socket_io_packet_ptr = std::shared_ptr<socket_io_packet>;
class socket_io_websocket_core;
using socket_io_websocket_core_ptr  = std::shared_ptr<socket_io_websocket_core>;
using socket_io_websocket_core_wptr = std::weak_ptr<socket_io_websocket_core>;
/// socket 连接
class socket_io_core : public std::enable_shared_from_this<socket_io_core> {
  using signal_type = boost::signals2::signal<void(const nlohmann::json&)>;
  using signal_ptr  = std::shared_ptr<signal_type>;

  uuid sid_;
  std::shared_ptr<sid_ctx> ctx_;
  socket_io_websocket_core_wptr websocket_;
  std::string namespace_;
  /// 发送事件的信号
  std::map<std::string, signal_ptr> signal_map_;
  std::optional<boost::signals2::scoped_connection> on_emit_scoped_connection_{};
  std::optional<boost::signals2::scoped_connection> on_message_scoped_connection_{};
  void on_impl(const socket_io_packet_ptr&);

 public:
  /**
   *
   * @param in_ctx 总上下文
   * @param in_namespace 连接使用的名称空间
   * @param in_json 初次连接时的负载
   */
  explicit socket_io_core(
      const std::shared_ptr<sid_ctx>& in_ctx, socket_io_websocket_core_wptr in_websocket,
      const std::string& in_namespace, const nlohmann::json& in_json
  );
  // destructor
  ~socket_io_core()                                      = default;
  // copy constructor
  socket_io_core(const socket_io_core& other)            = default;
  socket_io_core& operator=(const socket_io_core& other) = default;
  // move constructor
  socket_io_core(socket_io_core&& other)                 = default;
  socket_io_core& operator=(socket_io_core&& other)      = default;

  const uuid& get_sid() const { return sid_; }
  const std::string& get_namespace() const { return namespace_; }
  void set_namespace(const std::string& in_namespace) {
    namespace_ = in_namespace;
    signal_map_.clear();
    connect();
  }
  nlohmann::json auth_{};

  void emit(const std::string& in_event, const nlohmann::json& in_data);
  template <typename Solt>
  auto on_message(std::string& in_event_name, Solt&& in_solt) {
    return signal_map_[in_event_name]->connect(in_solt);
  }

  /// 连接消息来源
  void connect();
};
}  // namespace doodle::socket_io
