//
// Created by td_main on 2023/7/5.
//

#pragma once

// clang-format off
#include <algorithm>
#include <atomic>
#include <cwchar>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include <windows.h>
#include <wil/result.h>
#include <sddl.h>
#include <unknwn.h>
#include <winternl.h>
#include <comutil.h>
#include <oleidl.h>
#include <ntstatus.h>
#include <cfapi.h>
// clang-format on
#include "doodle_core/logger/logger.h"
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/signals2.hpp>
namespace doodle {

class directory_watcher {
 public:
  using file_action      = std::tuple<DWORD, FSys::path>;
  using file_action_list = std::vector<file_action>;

  boost::signals2::signal<void(const file_action_list&)> on_read_directory_change;
  template <
      typename ExecutionContext,
      typename std::enable_if_t<std::is_convertible_v<ExecutionContext&, boost::asio::execution_context&>>* = nullptr>
  explicit directory_watcher(ExecutionContext& in_context, FSys::path in_child_path)
      : executor_{in_context.get_executor()}, child_path{std::move(in_child_path)} {
    init();
  }
  //  template <
  //      typename ExecutionContext,
  //      std::enable_if_t<!std::is_convertible_v<ExecutionContext&, boost::asio::execution_context&>>* = 0>
  //  explicit directory_watcher(ExecutionContext& in_context, FSys::path in_child_path)
  //      : executor_{in_context}, child_path{std::move(in_child_path)} {}

  ~directory_watcher() = default;

  void read_changes_async() {
    auto l_handle = [this](const boost::system::error_code& ec, std::size_t) {
      if (ec) {
        DOODLE_LOG_INFO(ec.what());
        return;
      }
      auto* notify = this->notify_.get();
      do {
        auto filename = std::wstring_view{notify->FileName, notify->FileNameLength / sizeof(wchar_t)};
        this->files_.emplace_back(notify->Action, child_path / filename);

        if (notify->NextEntryOffset) {
          notify =
              reinterpret_cast<FILE_NOTIFY_INFORMATION*>(reinterpret_cast<char*>(notify) + notify->NextEntryOffset);
        } else {
          notify = nullptr;
        }
      } while (notify);
      read_changes_async();
    };
    overlapped_ptr_ = std::make_shared<boost::asio::windows::overlapped_ptr>(executor_, l_handle);
    DWORD returned;
    BOOL const l_ok = ::ReadDirectoryChangesW(
        dir_, notify_.get(), sizeof(FILE_NOTIFY_INFORMATION) * 100, TRUE,
        FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME, &returned, overlapped_ptr_->get(), nullptr
    );
    int last_error = GetLastError();
    if (!l_ok && last_error != ERROR_IO_PENDING) {
      boost::system::error_code const ec{last_error, boost::system::system_category()};
      overlapped_ptr_->complete(ec, 0);
    } else {
      overlapped_ptr_->release();
      overlapped_ptr_->reset(executor_, l_handle);
    }
  }

  void cancel() { ::CancelIoEx(dir_, overlapped_ptr_->get()); }

  void on_sync_root_file_changes() {
    for (auto&& [l_action, l_path] : files_) {
      DWORD l_attr = ::GetFileAttributesW(l_path.generic_wstring().c_str());

      if (!(l_attr & FILE_ATTRIBUTE_DIRECTORY)) {
        auto placeholder{CreateFile(l_path.c_str(), 0, FILE_READ_DATA, nullptr, OPEN_EXISTING, 0, nullptr)};
        LARGE_INTEGER const offset = {};
        LARGE_INTEGER length{};
        length.QuadPart = MAXLONGLONG;
        if (l_attr & FILE_ATTRIBUTE_PINNED) {
          //          wprintf(L"Hydrating file %s\n", path.c_str());
          ::CfHydratePlaceholder(placeholder, offset, length, CF_HYDRATE_FLAG_NONE, nullptr);
        }
        //        else if (l_attr & FILE_ATTRIBUTE_UNPINNED) {
        //          //          wprintf(L"Dehydrating file %s\n", path.c_str());
        //          ::CfDehydratePlaceholder(placeholder, offset, length, CF_DEHYDRATE_FLAG_NONE, NULL);
        //        }
      }

      if (l_action == FILE_ACTION_ADDED) {
        if (FSys::is_directory(l_path)) {
        }
      }
    }
  }

 private:
  void init() {
    const size_t c_bufferSize = sizeof(FILE_NOTIFY_INFORMATION) * 100;
    notify_.reset(reinterpret_cast<FILE_NOTIFY_INFORMATION*>(new char[c_bufferSize]));

    dir_ = ::CreateFileW(
        child_path.generic_wstring().c_str(), FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, nullptr
    );
    THROW_LAST_ERROR_IF(!dir_);
    boost::system::error_code l_code{};
    boost::asio::use_service<boost::asio::detail::io_context_impl>(
        boost::asio::query(executor_, boost::asio::execution::context)
    )
        .register_handle(dir_, l_code);
    if (l_code) {
      throw_exception(std::system_error{l_code});
    }
  }

  FSys::path child_path;
  std::unique_ptr<FILE_NOTIFY_INFORMATION> notify_;
  std::vector<std::tuple<DWORD, FSys::path>> files_;
  std::shared_ptr<boost::asio::windows::overlapped_ptr> overlapped_ptr_;
  boost::asio::any_io_executor executor_;
  HANDLE dir_;
};

}  // namespace doodle
