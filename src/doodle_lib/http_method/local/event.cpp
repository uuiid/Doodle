//
// Created by TD on 25-1-23.
//

#include "event.h"

#include <doodle_lib/core/engine_io.h>
#include <doodle_lib/core/http/http_function.h>

namespace doodle::http::local {

void local_event_reg(http_route& in_route) {
  auto l_sid_ctx = std::make_shared<socket_io::sid_ctx>();

  socket_io::create_socket_io(in_route, l_sid_ctx);
}
}  // namespace doodle::http::local