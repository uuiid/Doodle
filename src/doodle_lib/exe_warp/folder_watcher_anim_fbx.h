#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::exe_warp {
class folder_watcher_anim_fbx {
 public:
  void watch(const FSys::path& in_path);
};
}  // namespace doodle::exe_warp