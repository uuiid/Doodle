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
#include <boost/exception/diagnostic_information.hpp>
#include <boost/lockfree/detail/uses_optional.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/system/error_code.hpp>

#include "core/core_set.h"
#include "http_client/kitsu_client.h"
#include <Windows.h>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fmt/format.h>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <system_error>
#include <utility>
#include <vector>
#include <wil/resource.h>
#include <wil/result.h>
#include <wil/result_macros.h>

namespace doodle {
namespace {
boost::system::error_code make_last_error_code() {
  boost::system::error_code error_code;
  error_code.assign(static_cast<int>(::GetLastError()), boost::system::system_category());
  return error_code;
}
}  // namespace

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

    if (native == INVALID_HANDLE_VALUE) throw_exception(std::system_error{make_last_error_code()});
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
          boost::asio::windows::overlapped_ptr overlapped{ex, std::move(l_h)};
          if (!directory_handle_.is_open())
            return overlapped.complete(boost::asio::error::make_error_code(boost::asio::error::operation_aborted), 0U);

          constexpr DWORD kDirectoryNotifyFilter = FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_LAST_WRITE;
          DWORD bytes_transferred                = 0;
          BOOL result                            = ::ReadDirectoryChangesW(
              directory_handle_.native_handle(), notify_buffer_.data(), static_cast<DWORD>(notify_buffer_.size()),
              FALSE, kDirectoryNotifyFilter, &bytes_transferred, overlapped.get(), nullptr
          );
          if (!result) return overlapped.complete(make_last_error_code(), 0U);

          overlapped.release();
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

  struct task_info {
    uuid project_id_;
    uuid task_id_;
    FSys::path file_name_;
  };

  std::map<FSys::path, task_info> path_task_map_{};

  void add_task_path(const folder_watcher_anim_fbx::watch_arg& in_arg) {
    path_task_map_[in_arg.path_.generic_string()] = {in_arg.project_id_, in_arg.task_id_, in_arg.file_name_};
  }

  void start_watch() {
    boost::asio::co_spawn(
        strand_, watch_loop(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(),
            boost::asio::consign(boost::asio::detached, shared_from_this(), self_->shared_from_this())
        )
    );
  }
  void stop_watch() {
    stopping_ = true;
    boost::asio::co_spawn(
        strand_, process_changed(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(),
            boost::asio::consign(boost::asio::detached, shared_from_this(), self_->shared_from_this())
        )
    );
  }

 private:
  std::atomic_bool stopping_{false};
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};
  boost::asio::steady_timer flush_timer_{strand_};
  std::set<FSys::path> changed_files_{};
  read_directory_changes_watcher_t watcher_{strand_};

  // 定时为一小时后处理
  void schedule_flush() {
    flush_timer_.expires_after(std::chrono::hours(1));
    flush_timer_.async_wait(
        boost::asio::bind_executor(
            strand_, [self = shared_from_this(), l_s = self_->shared_from_this()](const boost::system::error_code& ec) {
              if (ec == boost::asio::error::operation_aborted) return;
              if (ec) {
                SPDLOG_ERROR("flush_timer_ error: {}", ec.message());
                return;
              }
              boost::asio::co_spawn(
                  self->strand_, self->process_changed(),
                  boost::asio::bind_cancellation_slot(
                      app_base::Get().on_cancel.slot(),
                      boost::asio::consign(boost::asio::detached, self->shared_from_this(), l_s->shared_from_this())
                  )
              );
            }
        )
    );
  }

  bool is_processing_path(const FSys::path& in_path) const {
    if (in_path.extension() != ".ma") return false;
    return path_task_map_.contains(in_path.generic_string());
  }
  // 是否 小于 1 小时
  bool is_recently_changed(const FSys::path& in_path) const {
    return FSys::last_write_time(in_path) + std::chrono::hours(1) > FSys::file_time_type::clock::now() || stopping_;
  }

  // 全部处理
  boost::asio::awaitable<void> process_changed() {
    auto l_client = std::make_shared<kitsu::kitsu_client>(core_set::get_set().server_ip);
    for (auto begin = changed_files_.begin(); begin != changed_files_.end();) {
      if (is_recently_changed(*begin)) {
        co_await process_changed(*begin, l_client);
        begin = changed_files_.erase(begin);
      } else {
        ++begin;
      }
    }
  }

  boost::asio::awaitable<void> watch_loop() {
    watcher_.open(watch_path_);
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      std::size_t l_bytes_transferred = co_await watcher_.async_read_changes(boost::asio::use_awaitable);

      // 处理文件变更事件
      for (const auto& [path, action] : watcher_.parse_changes(l_bytes_transferred))
        if (is_processing_path(path)) changed_files_.insert(path);

      co_await process_changed();
      schedule_flush();
    }
  }
  // 开始处理文件
  boost::asio::awaitable<void> process_changed(
      const FSys::path& in_path, const std::shared_ptr<kitsu::kitsu_client>& in_client
  ) {
    // 小于 1 小时不处理
    if (is_processing_path(in_path)) co_return;
    auto l_task_id = path_task_map_[in_path];
    try {
      auto l_copy_path =
          core_set::get_set().get_cache_root(fmt::format("anim_maya/{}", core_set::get_set().get_uuid())) /
          l_task_id.file_name_;
      if (auto l_p = l_copy_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
      FSys::copy_file(in_path, l_copy_path, FSys::copy_options::overwrite_existing);
      // 上传文件
      co_await in_client->upload_shot_animation_maya(l_task_id.task_id_, l_copy_path);
      co_await in_client->run_export_anim_fbx_task(l_task_id.project_id_, l_task_id.task_id_);
    } catch (...) {
      SPDLOG_ERROR(boost::current_exception_diagnostic_information());
    }
  }
};

class folder_watcher_anim_fbx::impl {
 public:
  impl()  = default;
  ~impl() = default;

  folder_watcher_anim_fbx* self_{};
  boost::lockfree::spsc_queue<watch_arg> watch_queue_{1024};

  void begin_watch() {
    boost::asio::co_spawn(
        strand_, watch_loop(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, self_->shared_from_this())
        )
    );
  }

  void stop_all() {
    stopping_ = true;
    flush_timer_.cancel();
    boost::asio::co_spawn(
        strand_, process_changed(),
        boost::asio::bind_cancellation_slot(
            app_base::Get().on_cancel.slot(), boost::asio::consign(boost::asio::detached, self_->shared_from_this())
        )
    );
  }
  boost::asio::awaitable<std::vector<watch_arg>> get_watch_args() const {
    DOODLE_TO_EXECUTOR(strand_);
    std::vector<watch_arg> out = changed_files_;
    co_return out;
  }

 private:
  // 是否 大于 1 小时 小于 3 小时
  bool is_recently_changed(const FSys::path& in_path) const {
    auto l_new = FSys::file_time_type::clock::now();
    return FSys::exists(in_path) &&
           ((l_new - FSys::last_write_time(in_path) > 1h && l_new - FSys::last_write_time(in_path) < 3h) || stopping_);
  }

  std::atomic_bool stopping_{false};
  boost::asio::strand<boost::asio::io_context::executor_type> strand_{boost::asio::make_strand(g_io_context())};
  boost::asio::steady_timer flush_timer_{strand_};
  std::vector<watch_arg> changed_files_{};

  boost::asio::awaitable<void> watch_loop() {
    while ((co_await boost::asio::this_coro::cancellation_state).cancelled() == boost::asio::cancellation_type::none) {
      flush_timer_.expires_after(std::chrono::hours(1));
      co_await flush_timer_.async_wait(boost::asio::use_awaitable);
      while (!watch_queue_.empty())
        if (auto l_v = watch_queue_.pop(boost::lockfree::uses_optional); l_v) changed_files_.push_back(*l_v);
      co_await process_changed();
    }
  }
  boost::asio::awaitable<void> process_changed() {
    auto l_client = std::make_shared<kitsu::kitsu_client>(core_set::get_set().server_ip);
    for (auto begin = changed_files_.begin(); begin != changed_files_.end();) {
      if (is_recently_changed(begin->path_)) {
        co_await process_changed(*begin, l_client);
        begin = changed_files_.erase(begin);
      } else {
        ++begin;
      }
    }
  }

  // 开始处理文件
  boost::asio::awaitable<void> process_changed(
      const watch_arg& in_arg, const std::shared_ptr<kitsu::kitsu_client>& in_client
  ) {
    // 小于 1 小时不处理
    if (is_recently_changed(in_arg.path_)) co_return;
    try {
      auto l_copy_path =
          core_set::get_set().get_cache_root(fmt::format("anim_maya/{}", core_set::get_set().get_uuid())) /
          in_arg.file_name_;
      if (auto l_p = l_copy_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
      FSys::copy_file(in_arg.path_, l_copy_path, FSys::copy_options::overwrite_existing);
      if (auto l_file_info = co_await in_client->upload_shot_animation_maya_heap(in_arg.task_id_, l_copy_path);
          l_file_info.exist_ &&
          l_file_info.updated_time_ >= chrono::clock_cast<chrono::utc_clock>(FSys::last_write_time(in_arg.path_))) {
        SPDLOG_INFO("文件 {} 已经存在且未修改, 跳过上传", in_arg.path_.generic_string());
        co_return;
      }
      // 上传文件
      co_await in_client->upload_shot_animation_maya(in_arg.task_id_, l_copy_path);
      co_await in_client->run_export_anim_fbx_task(in_arg.project_id_, in_arg.task_id_);
    } catch (...) {
      SPDLOG_ERROR(boost::current_exception_diagnostic_information());
    }
  }
};

folder_watcher_anim_fbx::folder_watcher_anim_fbx() : impl_(std::make_unique<impl>()) { impl_->self_ = this; }
folder_watcher_anim_fbx::~folder_watcher_anim_fbx() { stop_watch(); }

void folder_watcher_anim_fbx::watch(const std::vector<watch_arg>& in_arg) {
  for (const auto& arg : in_arg) impl_->watch_queue_.push(arg);
}

void folder_watcher_anim_fbx::stop_watch() { impl_->stop_all(); }
boost::asio::awaitable<std::vector<folder_watcher_anim_fbx::watch_arg>>
folder_watcher_anim_fbx::get_watch_args() const {
  return impl_->get_watch_args();
}

}  // namespace doodle::exe_warp