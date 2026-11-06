//
// Created by TD on 2026-11-6.
//

#include "face_capture.h"

#include <doodle_lib/core/face_capture/ws_stream_runner.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>

namespace doodle::http::local {

bool face_capture_stream::has_websocket() const { return true; }

void face_capture_stream::websocket_init(session_data_ptr in_handle) {}

void face_capture_stream::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {
  runner_ = std::make_shared<face_capture::ws_stream_runner>(std::move(in_stream), in_handle->logger_);
  runner_->start();
}

}  // namespace doodle::http::local
