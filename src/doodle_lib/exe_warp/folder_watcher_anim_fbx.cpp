#include "folder_watcher_anim_fbx.h"

#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"

#include <doodle_lib/core/app_base.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/bind_cancellation_slot.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/this_coro.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/windows/basic_object_handle.hpp>
#include <boost/asio/windows/object_handle.hpp>
#include <boost/asio/windows/overlapped_ptr.hpp>
#include <boost/system/detail/error_code.hpp>

#include <Windows.h>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/result_macros.h>

namespace doodle {
template <typename Executor = boost::asio::any_io_executor>
class read_directory_changes_watcher {
 public:
  using executor_type = Executor;

  /// Rebinds the handle type to another executor.
  template <typename Executor1>
  struct rebind_executor {
    /// The handle type when rebound to the specified executor.
    using other = read_directory_changes_watcher<Executor1>;
  };
  using native_handle_type = boost::asio::windows::object_handle::native_handle_type;
  using lowest_layer_type  = read_directory_changes_watcher;

  explicit read_directory_changes_watcher(const executor_type& ex) : directory_handle_(ex) {}
  template <typename ExecutionContext>
  explicit read_directory_changes_watcher(
      ExecutionContext& context,
      typename boost::asio::constraint_t<
          boost::asio::is_convertible<ExecutionContext&, boost::asio::execution_context&>::value,
          boost::asio::defaulted_constraint>::type = boost::asio::defaulted_constraint()
  )
      : directory_handle_(context) {}

  void open(const FSys::path& in_path) {
    watch_path_ = in_path;
    watch_path_.make_preferred();
    watch_path_   = FSys::absolute(watch_path_);

    HANDLE native = ::CreateFileW(
        watch_path_.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr
    );

    if (native == INVALID_HANDLE_VALUE)
      throw_exception(
          std::system_error{
              boost::system::error_code{static_cast<int>(::GetLastError()), boost::system::system_category()}
          }
      );
    directory_handle_.assign(native);
  }

  ~read_directory_changes_watcher() = default;

  bool is_open() const noexcept { return directory_handle_.is_open(); }

  void close() { directory_handle_.close(); }

  template <typename CompletionToken>
  auto async_read_changes(CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken, void(boost::system::error_code, std::size_t)>(
        [this](auto&& handler) mutable {
          using handler_type = std::decay_t<decltype(handler)>;
          handler_type l_h{std::forward<decltype(handler)>(handler)};
          // 获取默认执行器
          auto ex = boost::asio::get_associated_executor(l_h, directory_handle_.get_executor());
          if (!directory_handle_.is_open()) {
            boost::asio::post(ex, [in_h = std::move(l_h)]() mutable {
              in_h(boost::asio::error::make_error_code(boost::asio::error::operation_aborted), 0U);
            });
            return;
          }
          constexpr DWORD kDirectoryNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
          auto overlapped = std::make_shared<boost::asio::windows::overlapped_ptr>(directory_handle_, std::move(l_h));
          DWORD bytes_transferred = 0;
          BOOL result             = ::ReadDirectoryChangesW(
              directory_handle_.native_handle(), notify_buffer_.data(), static_cast<DWORD>(notify_buffer_.size()),
              FALSE, kDirectoryNotifyFilter, &bytes_transferred, overlapped->overlapped(), nullptr
          );
          if (!result) {
            auto ec = boost::system::error_code{static_cast<int>(::GetLastError()), boost::system::system_category()};
            boost::asio::post(ex, [in_h = std::move(overlapped->handler()), ec]() mutable { in_h(ec, 0U); });
            return;
          }
          overlapped->release();
        },
        token
    );
  }
  // 辅助函数, 将缓冲区中的文件变更事件解析为路径和变更类型
  auto parse_changes(std::size_t bytes_transferred) {
    // 辅助解析缓冲区的迭代器类, 兼容 begin , end
    // template <typename Executor = boost::asio::any_io_executor>
    struct buffer_iterator {
      read_directory_changes_watcher<Executor>* watcher;
      FILE_NOTIFY_INFORMATION* current;

      bool operator!=(const buffer_iterator& other) const { return current != other.current; }
      buffer_iterator& operator++() {
        if (current->NextEntryOffset == 0) {
          current = nullptr;
        } else {
          current = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(
              reinterpret_cast<std::byte*>(current) + current->NextEntryOffset
          );
        }
        return *this;
      }
      buffer_iterator operator++(int) {
        buffer_iterator temp = *this;
        ++(*this);
        return temp;
      }
      std::pair<FSys::path, DWORD> operator*() const {
        std::wstring file_name(current->FileName, current->FileNameLength / sizeof(WCHAR));
        auto l_full_path = watcher->watch_path_ / file_name;
        return {std::move(l_full_path), current->Action};
      }
      buffer_iterator begin() { return *this; }
      buffer_iterator end() { return {nullptr}; }
    };
    return buffer_iterator{this, reinterpret_cast<FILE_NOTIFY_INFORMATION*>(notify_buffer_.data())};
  }

 private:
  static constexpr std::size_t kNotifyBufferSize = 64 * 1024;
  boost::asio::windows::basic_object_handle<Executor> directory_handle_;
  FSys::path watch_path_;
  std::array<std::byte, kNotifyBufferSize> notify_buffer_{};
};

using read_directory_changes_watcher_t = read_directory_changes_watcher<>;
}  // namespace doodle

namespace doodle::exe_warp {

class folder_watcher_anim_fbx_folder : public std::enable_shared_from_this<folder_watcher_anim_fbx_folder> {
 public:
  folder_watcher_anim_fbx_folder()  = default;
  ~folder_watcher_anim_fbx_folder() = default;

  folder_watcher_anim_fbx* self_{};
  FSys::path watch_path_;
  std::map<FSys::path, uuid> path_task_map_{};

  void add_task_path(const folder_watcher_anim_fbx::watch_arg& in_arg) {
    path_task_map_[in_arg.path_.generic_string()] = in_arg.task_id_;
  }

 private:
  std::atomic_bool started_{false};
  std::atomic_bool stop_requested_{false};
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};
  std::unique_ptr<boost::asio::windows::object_handle> directory_handle_{};
  boost::asio::steady_timer flush_timer_{strand_};
  std::map<FSys::path, FSys::file_time_type> changed_files_{};

  bool is_processing_path(const FSys::path& in_path) const {
    if (in_path.extension() != ".ma") return false;
    return path_task_map_.contains(in_path.generic_string());
  }
};

class folder_watcher_anim_fbx::impl {
 public:
  impl()  = default;
  ~impl() = default;

  folder_watcher_anim_fbx* self_{};
  std::vector<std::shared_ptr<folder_watcher_anim_fbx_folder>> folders_;

  void create_watcher(const FSys::path& in_root_path, const std::vector<watch_arg>& in_arg) {
    auto folder         = std::make_shared<folder_watcher_anim_fbx_folder>();
    folder->self_       = self_;
    folder->watch_path_ = in_root_path;
    for (const auto& arg : in_arg) folder->add_task_path(arg);
    folders_.emplace_back(std::move(folder));
  }

  void stop_all() {}
};

folder_watcher_anim_fbx::folder_watcher_anim_fbx() : impl_(std::make_unique<impl>()) { impl_->self_ = this; }
folder_watcher_anim_fbx::~folder_watcher_anim_fbx() { stop_watch(); }

void folder_watcher_anim_fbx::watch(const FSys::path& in_root_path, const std::vector<watch_arg>& in_arg) {
  impl_->create_watcher(in_root_path, in_arg);
}

void folder_watcher_anim_fbx::stop_watch() { impl_->stop_all(); }
}  // namespace doodle::exe_warp