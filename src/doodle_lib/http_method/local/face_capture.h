//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/capture_session.h>
#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/http/http_function.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/local/local.h>

#include <boost/uuid/uuid.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace doodle::http::local {

using face_capture_session_holder = face_capture::capture_session_holder;

class face_capture_selected_device {
 public:
  face_capture_selected_device();

  [[nodiscard]] std::int32_t index() const;

  void select(std::int32_t in_index);

 private:
  struct impl;
  std::shared_ptr<impl> impl_ptr_{};
};

DOODLE_HTTP_FUN_C(face_capture_devices, local_http_fun)
std::shared_ptr<face_capture_selected_device> selected_{};
std::shared_ptr<face_capture_session_holder> holder_{};

explicit face_capture_devices(
    std::shared_ptr<face_capture_selected_device> in_selected,
    std::shared_ptr<face_capture_session_holder> in_holder
)
    : base_type(), selected_(std::move(in_selected)), holder_(std::move(in_holder)) {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/devices")
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_stream, local_http_fun)
boost::uuids::uuid session_id_{};
std::shared_ptr<face_capture_selected_device> selected_{};
std::shared_ptr<face_capture_session_holder> holder_{};

explicit face_capture_stream(
    std::shared_ptr<face_capture_selected_device> in_selected,
    std::shared_ptr<face_capture_session_holder> in_holder
)
    : base_type(), selected_(std::move(in_selected)), holder_(std::move(in_holder)) {}

[[nodiscard]] bool has_websocket() const override;

void websocket_init(session_data_ptr in_handle) override;

void websocket_callback(
    boost::beast::websocket::stream<tcp_stream_type> in_stream, session_data_ptr in_handle
) override;

DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/stream/{session_id_}", session_id_)
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recordings, local_http_fun)
std::shared_ptr<face_capture_session_holder> holder_{};

explicit face_capture_recordings(std::shared_ptr<face_capture_session_holder> in_holder)
    : base_type(), holder_(std::move(in_holder)) {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings")
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording_stop, local_http_fun)
boost::uuids::uuid session_id_{};
std::shared_ptr<face_capture_session_holder> holder_{};

explicit face_capture_recording_stop(std::shared_ptr<face_capture_session_holder> in_holder)
    : base_type(), holder_(std::move(in_holder)) {}

DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}/stop", session_id_)
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording, local_http_fun)
boost::uuids::uuid session_id_{};
std::shared_ptr<face_capture_session_holder> holder_{};

explicit face_capture_recording(std::shared_ptr<face_capture_session_holder> in_holder)
    : base_type(), holder_(std::move(in_holder)) {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}", session_id_)
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording_video, local_http_fun)
boost::uuids::uuid session_id_{};

face_capture_recording_video() : base_type() {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}/video", session_id_)
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording_curves, local_http_fun)
boost::uuids::uuid session_id_{};

face_capture_recording_curves() : base_type() {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}/curves", session_id_)
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::local
