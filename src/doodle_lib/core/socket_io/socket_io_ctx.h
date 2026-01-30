//
// Created by TD on 25-2-17.
//

#pragma once
#include "doodle_core/core/global_function.h"
#include <doodle_core/core/cancellation_signals.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/socket_io/core_enum.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/strand.hpp>
#include <boost/signals2/signal.hpp>

#include <memory>

namespace doodle ::socket_io {
class socket_io_core;
struct socket_io_packet;
class sid_data;
using socket_io_packet_ptr = std::shared_ptr<socket_io_packet>;

/// 握手数据
struct handshake_data {
  uuid sid_{};
  std::vector<transport_type> upgrades_{};
  chrono::milliseconds ping_interval_{};
  chrono::milliseconds ping_timeout_{};
  std::int32_t max_payload_{};
  // to json
  friend void to_json(nlohmann::json& nlohmann_json_j, const handshake_data& nlohmann_json_t) {
    nlohmann_json_j["sid"]          = nlohmann_json_t.sid_;
    nlohmann_json_j["upgrades"]     = nlohmann_json_t.upgrades_;
    nlohmann_json_j["pingInterval"] = nlohmann_json_t.ping_interval_;
    nlohmann_json_j["pingTimeout"]  = nlohmann_json_t.ping_timeout_;
    nlohmann_json_j["maxPayload"]   = nlohmann_json_t.max_payload_;
  }
};

class sid_ctx {
 public:
  struct signal_type {
    friend class sid_ctx;

   private:
    boost::signals2::signal<void(const std::shared_ptr<socket_io_core>&)> on_connect_;
    /// 服务器接收到消息时调用
    boost::signals2::signal<void(const socket_io_packet_ptr&)> on_message_;

   public:
    using emit_signal_type    = boost::signals2::signal<void(const socket_io_packet_ptr&)>;
    using message_signal_type = boost::signals2::signal<void(const socket_io_packet_ptr&)>;
    using emit_solt_type      = emit_signal_type::slot_type;
    using message_solt_type   = message_signal_type::slot_type;

    template <typename Solt>
    auto on_connect(Solt&& in_solt) {
      return on_connect_.connect(in_solt);
    }

    /// 在服务器接受到消息时调用的槽
    template <typename Solt>
    auto on_message(Solt&& in_solt) {
      return on_message_.connect(in_solt);
    }
    /// 发出消息
    void message(const socket_io_packet_ptr& in_data);
  };

 private:
  using signal_type_ptr = std::shared_ptr<signal_type>;

  std::map<uuid, std::shared_ptr<sid_data>> sid_map_{};
  std::map<std::string, socket_io_packet_ptr> socket_map_{};
  std::map<std::string, signal_type_ptr> signal_map_{};
  // decltype(boost::asio::make_strand(g_io_context()))
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{};

  void clear_timeout_sid();
  void emit_impl(const socket_io_packet_ptr& in_data) const;

 public:
  handshake_data handshake_data_{};
  sid_ctx();

  void register_namespace(const std::string& in_namespace);

  /// 线程安全
  boost::asio::awaitable<std::shared_ptr<sid_data>> generate();
  /// 线程安全
  boost::asio::awaitable<std::shared_ptr<sid_data>> get_sid(uuid in_sid) const;
  /// 线程安全
  boost::asio::awaitable<signal_type_ptr> on(std::string in_namespace);

  void emit_connect(const std::shared_ptr<socket_io_core>& in_data) const;
  /// 发出信号
  void emit(const socket_io_packet_ptr& in_data) const;
  /// 针对单条连接发出信号
  void emit_to_sid(const socket_io_packet_ptr& in_data, const uuid& in_sid) const;

  boost::asio::awaitable<bool> has_register(std::string in_namespace) const;
  void clear();

  cancellation_signals on_cancel;
};
}  // namespace doodle::socket_io
