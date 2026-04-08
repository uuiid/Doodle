#include "folder_watcher_anim_fbx.h"

#include <doodle_lib/core/app_base.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/strand.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/scope/scope_exit.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <vector>
#include <wil/filesystem.h>

namespace doodle::exe_warp {

class folder_watcher_anim_fbx_folder : public std::enable_shared_from_this<folder_watcher_anim_fbx_folder> {
 public:
  folder_watcher_anim_fbx_folder()  = default;
  ~folder_watcher_anim_fbx_folder() = default;

  folder_watcher_anim_fbx* self_{};
  FSys::path watch_path_;
  boost::lockfree::spsc_queue<FSys::path, boost::lockfree::capacity<1024>> message_queue_;
  wil::unique_folder_change_reader change_notification_;

  void push_message(const FSys::path& in_path) {
    message_queue_.push(in_path);
    if (!watching_)
      boost::asio::co_spawn(
          strand_, process_changes(),
          boost::asio::bind_cancellation_slot(
              app_base::Get().on_cancel.slot(),
              boost::asio::consign(boost::asio::detached, self_->shared_from_this(), shared_from_this())
          )
      );
  }

 private:
  std::atomic_bool watching_{false};
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};
  struct path_info {
    FSys::path path_;
    FSys::file_time_type last_write_time_;
  };
  std::map<FSys::path, path_info> changed_files_;

  // 处理更改的目录
  boost::asio::awaitable<void> process_changes() {
    if (message_queue_.empty()) co_return;
    watching_ = true;
    boost::scope::scope_exit l_{[this]() { watching_ = false; }};
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      if (message_queue_.empty()) co_return;

      FSys::path l_path{};
      while (message_queue_.pop(l_path)) {
        changed_files_[l_path] = {l_path, FSys::last_write_time(l_path)};
      }
      for (auto& [path, file_info] : changed_files_) {
        // 已经修改文件一小时后, 进行操作, 避免文件频繁修改导致的重复操作
        if (chrono::utc_clock::now() - chrono::clock_cast<chrono::utc_clock>(file_info.last_write_time_) >
            chrono::hours(1)) {
        }
      }
    }
  }
};

class folder_watcher_anim_fbx::impl {
 public:
  impl()  = default;
  ~impl() = default;

  folder_watcher_anim_fbx* self_{};
  std::vector<std::shared_ptr<folder_watcher_anim_fbx_folder>> folders_;

  void create_watcher(const FSys::path& in_path) {
    folders_.emplace_back(std::make_shared<folder_watcher_anim_fbx_folder>());
    folders_.back()->self_                = self_;
    folders_.back()->watch_path_          = in_path;
    folders_.back()->change_notification_ = wil::make_folder_change_reader(
        folders_.back()->watch_path_.c_str(), true, wil::FolderChangeEvents::All,
        [this, self = self_->shared_from_this(),
         folder = folders_.back()](wil::FolderChangeEvent in_event, PCWSTR in_path) {
          FSys::path l_path{in_path};
          switch (in_event) {
            case wil::FolderChangeEvent::Modified:
              if (l_path.extension() == ".ma") folder->push_message(l_path);
              break;
            default:
              break;
          }
        }
    );
  }
};

folder_watcher_anim_fbx::folder_watcher_anim_fbx() : impl_(std::make_unique<impl>()) { impl_->self_ = this; }
folder_watcher_anim_fbx::~folder_watcher_anim_fbx() = default;

void folder_watcher_anim_fbx::watch(const FSys::path& in_path) { impl_->create_watcher(in_path); }
}  // namespace doodle::exe_warp