#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::exe_warp {
class folder_watcher_anim_fbx {
  
 public:
  folder_watcher_anim_fbx()  = default;
  ~folder_watcher_anim_fbx() = default;

  void watch(const FSys::path& in_path);

  void stop_watch();
};
}  // namespace doodle::exe_warp