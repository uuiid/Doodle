#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <filesystem>
#include <vector>

namespace doodle::exe_warp {
class folder_watcher_anim_fbx : public std::enable_shared_from_this<folder_watcher_anim_fbx> {
  class impl;
  std::unique_ptr<impl> impl_;

 public:
  folder_watcher_anim_fbx();
  ~folder_watcher_anim_fbx();

  struct watch_arg {
    FSys::path path_;
    uuid task_id_;
  };

  void watch(const FSys::path& in_root_path, const std::vector<watch_arg>& in_task_id);

  void stop_watch();
};
}  // namespace doodle::exe_warp