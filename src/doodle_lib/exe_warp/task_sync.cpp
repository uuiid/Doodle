#include "doodle_lib/exe_warp/task_sync.h"

#include "doodle_core/core/file_sys.h"
#include "doodle_core/exception/exception.h"

#include <doodle_lib/exe_warp/ue_exe.h>

#include <filesystem>

namespace doodle {

task_sync::args& task_sync::args::operator+=(const args& in_other) {
  update_file_list_.insert(
      update_file_list_.end(), in_other.update_file_list_.begin(), in_other.update_file_list_.end()
  );
  download_file_list_.insert(
      download_file_list_.end(), in_other.download_file_list_.begin(), in_other.download_file_list_.end()
  );

  // clear duplicate

  if (auto l_update_end = std::unique(update_file_list_.begin(), update_file_list_.end());
      l_update_end != update_file_list_.end())
    update_file_list_.erase(l_update_end, update_file_list_.end());

  if (auto l_download_end = std::unique(download_file_list_.begin(), download_file_list_.end());
      l_download_end != download_file_list_.end())
    download_file_list_.erase(l_download_end, download_file_list_.end());
  return *this;
}

boost::asio::awaitable<void> task_sync::run() {
  args l_total_args;
  for (auto&& l_task_id : task_ids_) {
    auto l_file_sync_info = co_await kitsu_client_->get_task_sync(l_task_id);
    auto l_arg            = l_file_sync_info.get<args>();
    for (auto&& l_info : l_arg.update_file_list_) l_info.task_id_ = l_task_id;
    l_total_args += l_arg;
  }
  auto l_root = core_set::get_set().user_work_root_;

  if (download_)
    for (auto&& l_info : l_total_args.download_file_list_) FSys::copy_diff(l_info.from_path_, l_root / l_info.to_path_);

  if (update_) {
    for (auto&& l_info : l_total_args.update_file_list_) {
      auto l_ue_project_dir = ue_exe_ns::find_ue_project_file(l_root / l_info.from_path_).parent_path().parent_path();
      DOODLE_CHICK(!l_ue_project_dir.empty(), "未找到本地UE项目 {}", (l_root / l_info.from_path_));
      auto l_local_path = l_root / l_info.from_path_;
      if (!FSys::exists(l_local_path)) continue;
      if (FSys::is_directory(l_local_path))
        for (auto&& p : FSys::recursive_directory_iterator(l_local_path)) {
          if (p.is_directory()) continue;
          auto l_relative_path = p.path().lexically_relative(l_ue_project_dir);
          auto l_remote_path   = l_info.to_path_ / p.path().lexically_relative(l_local_path);
          if (!FSys::is_old_file(p.path(), l_remote_path))
            co_await kitsu_client_->upload_shot_animation_ue(
                l_info.task_id_, p.path(), l_relative_path.generic_string()
            );
        }
      else {
        auto l_relative_path = l_local_path.lexically_relative(l_ue_project_dir);
        if (!FSys::is_old_file(l_local_path, l_info.to_path_))
          co_await kitsu_client_->upload_shot_animation_ue(
              l_info.task_id_, l_local_path, l_relative_path.generic_string()
          );
      };
    }
  }

  co_return;
}
}  // namespace doodle