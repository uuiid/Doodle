//
// Created by TD on 25-1-22.
//

#pragma once

#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio/experimental/channel.hpp>
#include <boost/asio/experimental/concurrent_channel.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/url/url.hpp>

#include <cpp-base64/base64.h>
namespace doodle::socket_io {

class socket_io_core;
struct socket_io_packet;
using socket_io_packet_ptr = std::shared_ptr<socket_io_packet>;
enum class engine_io_packet_type : std::int8_t { open = 0, close, ping, pong, message, upgrade, noop };

enum class transport_type : std::int8_t { unknown, polling, websocket };

NLOHMANN_JSON_SERIALIZE_ENUM(
    transport_type, {{transport_type::unknown, "unknown"},
                     {transport_type::polling, "polling"},
                     {transport_type::websocket, "websocket"}}
)

struct query_data {
  std::int8_t EIO_{0};
  transport_type transport_;
  uuid sid_{};
};

query_data parse_query_data(const boost::urls::url& in_url);
inline std::string dump_message(
    const std::string& in_data, engine_io_packet_type in_type = engine_io_packet_type::message
) {
  return fmt::format("{}{}", static_cast<std::int8_t>(in_type), in_data);
}

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
class sid_ctx;
class sid_data {
 public:
  explicit sid_data(sid_ctx* in_ctx, uuid in_sid = core_set::get_set().get_uuid())
      : ctx_{in_ctx},
        sid_{in_sid},
        last_time_{std::chrono::system_clock::now()},
        is_upgrade_to_websocket_{false},
        close_{false} {}

  bool is_upgrade_to_websocket() const;
  bool is_timeout() const;
  void update_sid_time();
  void upgrade_to_websocket() { is_upgrade_to_websocket_ = true; }
  void close();
  uuid get_sid() const { return sid_; }

  std::shared_ptr<void> get_lock();
  bool is_locked() const;

  boost::asio::awaitable<std::string> async_event();

 private:
  struct lock_type {
    sid_data* data_;
    explicit lock_type(sid_data* in_data) : data_{in_data} { ++data_->lock_count_; }
    ~lock_type() { --data_->lock_count_; }
  };
  friend class sid_ctx;
  sid_ctx* ctx_;
  const uuid sid_;
  std::atomic<chrono::sys_time_pos> last_time_;
  std::atomic_bool is_upgrade_to_websocket_;
  std::atomic_int lock_count_;
  std::atomic_bool close_;
};
class sid_ctx {
  struct signal_type {
    boost::signals2::signal<void(const std::shared_ptr<socket_io_core>&)> on_connect_;
    boost::signals2::signal<void(const socket_io_packet_ptr&)> on_message_;
  };
  using signal_type_ptr = std::shared_ptr<signal_type>;
  /// 线程锁
  mutable std::shared_mutex mutex_;

  std::map<uuid, std::shared_ptr<sid_data>> sid_map_{};
  std::map<std::string, signal_type_ptr> signal_map_{};

 public:
  handshake_data handshake_data_{};
  sid_ctx();
  /// 线程安全
  std::shared_ptr<sid_data> generate();
  /// 线程安全
  std::shared_ptr<sid_data> get_sid(const uuid& in_sid) const;

  /// 线程不安全
  signal_type_ptr on(const std::string& in_namespace);

  template <typename Solt>
  auto on_connect(Solt&& in_solt) {
    return signal_map_.at({})->on_connect_.connect(in_solt);
  }
  template <typename Solt>
  auto on_message(Solt&& in_solt) {
    return signal_map_.at({})->on_message_.connect(in_solt);
  }

  void emit_connect(const std::shared_ptr<socket_io_core>& in_data) const;
  void emit(const socket_io_packet_ptr& in_data) const;

  bool has_register(const std::string& in_namespace) const { return signal_map_.contains(in_namespace); }
};
inline engine_io_packet_type parse_engine_packet(const std::string& in_str) {
  return in_str.empty() ? engine_io_packet_type::noop : num_to_enum<engine_io_packet_type>(in_str.front() - '0');
}
/// 是多个包
inline bool is_multi_packet(const std::string& in_data) {
  if (auto l_it = in_data.find('\x1e'); l_it != in_data.npos) return true;
  return false;
}
inline std::vector<std::string> split_multi_packet(const std::string& in_data) {
  std::vector<std::string> l_vec{};
  return boost::split(l_vec, in_data, boost::is_any_of("\x1e"));
}
inline bool is_binary_packet(const std::string& in_data) { return in_data.front() == 'b'; }
/// 解码二进制包
inline std::string decode_binary_packet(const std::string& in_data) {
  if (!is_binary_packet(in_data)) return in_data;
  std::string_view l_str{in_data.data() + 1, in_data.size() - 1};
  return base64_decode(l_str);
}

}  // namespace doodle::socket_io