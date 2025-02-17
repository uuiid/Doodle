//
// Created by TD on 25-2-17.
//
#pragma once
#include <doodle_core/doodle_core_fwd.h>
namespace doodle::socket_io {
enum class engine_io_packet_type : std::int8_t { open = 0, close, ping, pong, message, upgrade, noop };

enum class transport_type : std::int8_t { unknown, polling, websocket };

NLOHMANN_JSON_SERIALIZE_ENUM(
    transport_type, {{transport_type::unknown, "unknown"},
                     {transport_type::polling, "polling"},
                     {transport_type::websocket, "websocket"}}
)

enum class socket_io_packet_type : std::uint8_t {
  connect       = 0,
  disconnect    = 1,
  event         = 2,
  ack           = 3,
  connect_error = 4,
  binary_event  = 5,
  binary_ack    = 6
};

}  // namespace doodle::socket_io
