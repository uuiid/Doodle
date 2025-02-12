//
// Created by TD on 25-1-23.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_session_data.h>

namespace doodle::socket_io {
class sid_ctx;
class sid_data;
enum class socket_io_packet_type : std::uint8_t {
  connect       = 0,
  disconnect    = 1,
  event         = 2,
  ack           = 3,
  connect_error = 4,
  binary_event  = 5,
  binary_ack    = 6
};

struct socket_io_packet {
  socket_io_packet_type type_;
  std::string namespace_;
  std::string id_;
  std::size_t binary_count_{};
  nlohmann::json json_data_{};
  std::vector<std::string> binary_data_{};

  // 从字符串中解析
  static socket_io_packet parse(const std::string& in_str);
  std::string dump();
};

void create_socket_io(
    http::http_route& in_route, const std::shared_ptr<sid_ctx>& in_sid_ctx, const std::string& in_path = "/socket.io/"
);

}  // namespace doodle::socket_io