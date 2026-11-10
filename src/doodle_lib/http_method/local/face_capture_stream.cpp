//
// Created by TD on 2026-11-6.
//

#include "face_capture.h"

#include <doodle_lib/core/face_capture/ws_outlet.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>

namespace doodle::http::local {

bool face_capture_stream::has_websocket() const { return true; }

void face_capture_stream::websocket_init(session_data_ptr in_handle) {}

void face_capture_stream::websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) {
  std::make_shared<face_capture::ws_outlet>(std::move(in_stream), selected_ ? selected_->index() : 0, holder_)
      ->start();
}

}  // namespace doodle::http::local
