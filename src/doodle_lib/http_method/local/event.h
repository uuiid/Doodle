//
// Created by TD on 25-1-23.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io.h>

namespace doodle::http::local {

class local_event : public socket_io::event_base {
 public:
  std::optional<socket_io::socket_io_packet> get_last_event() const override;
  void event(const socket_io::socket_io_packet& in_packet) override;
};

void local_event_reg(http_route& in_route);
}  // namespace doodle::http::local
