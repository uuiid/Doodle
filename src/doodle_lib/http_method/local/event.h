//
// Created by TD on 25-1-23.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io.h>

namespace doodle::http::local {

void local_event_reg(http_route& in_route);
}  // namespace doodle::http::local
