//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/ws_stream_runner.h>
#include <doodle_lib/core/http/http_function.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/local/local.h>

#include <boost/uuid/uuid.hpp>

#include <memory>

namespace doodle::http::local {

DOODLE_HTTP_FUN_C(face_capture_stream, local_http_fun)
boost::uuids::uuid session_id_{};
std::shared_ptr<face_capture::ws_stream_runner> runner_{};

face_capture_stream() : base_type() {}

[[nodiscard]] bool has_websocket() const override;

void websocket_init(session_data_ptr in_handle) override;

void websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) override;

DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/stream/{session_id_}")
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::local
