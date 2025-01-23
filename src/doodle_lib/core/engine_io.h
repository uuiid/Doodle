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
struct query_data {
  std::int8_t EIO_{0};
  transport_type transport_;
  std::string sid_;
};

tl::expected<query_data, std::string> parse_query_data(const boost::urls::url& in_url);

class DOODLE_CORE_API engine_io {
 public:
  engine_io() = default;
  explicit engine_io(const std::string& in_data) { parse(in_data); }
  void parse(const std::string& in_data);
  engine_io_packet_type packet_type_;
  std::string data_;
  /// 生成回复
  std::string reply(std::string&& in_data);
};

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