//
// Created by TD on 25-1-23.
//

#pragma once

#include <doodle_core/core/co_queue.h>
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>

namespace doodle::socket_io {
DOODLE_HTTP_FUN(socket_io_http)
using session_data_ptr = std::shared_ptr<http::session_data>;
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
void websocket_callback(
    boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
) override;
[[nodiscard]] bool has_websocket() const override;
std::shared_ptr<sid_ctx> sid_ctx_;
explicit socket_io_http(std::shared_ptr<sid_ctx> sid_ctx) : sid_ctx_(std::move(sid_ctx)) {}
void init();
std::string generate_register_reply() const;
};

}  // namespace doodle::socket_io