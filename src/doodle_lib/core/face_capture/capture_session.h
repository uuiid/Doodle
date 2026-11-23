//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_lib/core/face_capture/face_capture_types.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::face_capture {

struct capture_session {
  static FSys::path default_root();
};

}  // namespace doodle::face_capture
