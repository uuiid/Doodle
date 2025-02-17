//
// Created by TD on 25-2-17.
//

#pragma once

#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/socket_io/core_enum.h>

#include <boost/url/url.hpp>

#include <cpp-base64/base64.h>
namespace doodle::socket_io {

class socket_io_core;
struct socket_io_packet;
using socket_io_packet_ptr = std::shared_ptr<socket_io_packet>;

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