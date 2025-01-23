//
// Created by TD on 25-1-22.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <boost/url/url.hpp>

namespace doodle::socket_io {
enum class engine_io_packet_type : std::int8_t { open = 0, close, ping, pong, message, upgrade, noop };
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
// inline bool is_binary_packet(const std::string& in_data) { return in_data.front() == 'b'; }
inline bool is_binary_packet(std::string& in_data) {
  auto l_is = in_data.front() == 'b';
  if (l_is) in_data.erase(0, 1);
  return l_is;
}

}  // namespace doodle::socket_io