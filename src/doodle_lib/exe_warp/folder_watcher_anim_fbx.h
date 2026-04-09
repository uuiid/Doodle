#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <boost/asio/awaitable.hpp>

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
    uuid project_id_;
    uuid task_id_;
    FSys::path file_name_;
  };

  void watch(const std::vector<watch_arg>& in_task_id);

  void stop_watch();

  // 获取所有的监事对象
  boost::asio::awaitable<std::vector<watch_arg>> get_watch_args() const;
};
}  // namespace doodle::exe_warp