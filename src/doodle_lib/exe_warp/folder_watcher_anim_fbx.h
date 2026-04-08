#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>

namespace doodle::exe_warp {
class folder_watcher_anim_fbx : public std::enable_shared_from_this<folder_watcher_anim_fbx> {
  class impl;
  std::unique_ptr<impl> impl_;

 public:
  folder_watcher_anim_fbx();
  ~folder_watcher_anim_fbx();

  void watch(const FSys::path& in_path);

  void stop_watch();
};
}  // namespace doodle::exe_warp