//
// Created by TD on 25-1-22.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/url/url.hpp>

#include <cpp-base64/base64.h>
namespace doodle::socket_io {
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

class sid_ctx {
  /// 线程锁
  mutable std::shared_mutex mutex_;

 public:
  std::map<uuid, chrono::sys_time_pos> sid_time_map_{};

  handshake_data handshake_data_{};
  sid_ctx()
      : handshake_data_{
            .upgrades_      = {transport_type::websocket},
            .ping_interval_ = chrono::milliseconds{25000},
            .ping_timeout_  = chrono::milliseconds{20000},
            .max_payload_   = 1000000
        } {}

  /// 生成sid
  uuid generate_sid();
  /// 查询sid是否超时
  bool is_sid_timeout(const uuid& in_sid) const;

  void update_sid_time(const uuid& in_sid);
};

engine_io_packet_type parse_engine_packet(const std::string& in_str) {
  return num_to_enum<engine_io_packet_type>(in_str.front());
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