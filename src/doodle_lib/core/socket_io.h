//
// Created by TD on 25-1-23.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>

namespace doodle::socket_io {

void create_socket_io(
    http::http_route& in_route, const std::shared_ptr<sid_ctx>& in_sid_ctx, const std::string& in_path = "/socket.io/"
);

}  // namespace doodle::socket_io