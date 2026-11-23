//
// Created by TD on 2026-11-6.
//

#include "capture_session.h"

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/core/file_sys.h>

namespace doodle::face_capture {

FSys::path capture_session::default_root() {
  return core_set::get_set().get_cache_root(FSys::path{"face_capture"} / "sessions");
}

}  // namespace doodle::face_capture
