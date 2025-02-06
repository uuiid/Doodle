//
// Created by TD on 25-1-23.
//

#include "event.h"

#include <doodle_lib/core/http/http_function.h>

namespace doodle::http::local {
void local_event::event(const socket_io::socket_io_packet& in_packet) {}

std::optional<socket_io::socket_io_packet> local_event::get_last_event() const { return std::nullopt; }
void local_event_reg(http_route& in_route) {
  auto l_event = std::make_shared<local_event>();
  socket_io::socket_io_http l_http{l_event};
  l_http.reg(in_route);
}
}  // namespace doodle::http::local