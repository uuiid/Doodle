//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>
#include <doodle_lib/core/http/http_function.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/local/local.h>

#include <boost/uuid/uuid.hpp>

#include <cstdint>
#include <memory>
#include <string>

namespace doodle::face_capture {
class capture_session;
}

namespace doodle::http::local {

class face_capture_session_holder {
 public:
  face_capture_session_holder();

  [[nodiscard]] std::shared_ptr<face_capture::capture_session> session() const;

  void reset(std::shared_ptr<face_capture::capture_session> in_session);

 private:
  struct impl;
  std::shared_ptr<impl> impl_ptr_{};
};

DOODLE_HTTP_FUN_C(face_capture_recordings, local_http_fun)
std::shared_ptr<face_capture_session_holder> holder_{};

face_capture_recordings() : base_type() {}

explicit face_capture_recordings(std::shared_ptr<face_capture_session_holder> in_holder)
    : base_type(), holder_(std::move(in_holder)) {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings")
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording_stop, local_http_fun)
boost::uuids::uuid session_id_{};
std::shared_ptr<face_capture_session_holder> holder_{};

face_capture_recording_stop() : base_type() {}

explicit face_capture_recording_stop(std::shared_ptr<face_capture_session_holder> in_holder)
    : base_type(), holder_(std::move(in_holder)) {}

DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}/stop")
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording, local_http_fun)
boost::uuids::uuid session_id_{};
std::shared_ptr<face_capture_session_holder> holder_{};

face_capture_recording() : base_type() {}

explicit face_capture_recording(std::shared_ptr<face_capture_session_holder> in_holder)
    : base_type(), holder_(std::move(in_holder)) {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}")
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording_video, local_http_fun)
boost::uuids::uuid session_id_{};

face_capture_recording_video() : base_type() {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}/video")
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN_C(face_capture_recording_curves, local_http_fun)
boost::uuids::uuid session_id_{};

face_capture_recording_curves() : base_type() {}

DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/doodle/local/face_capture/recordings/{session_id_}/curves")
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http::local
