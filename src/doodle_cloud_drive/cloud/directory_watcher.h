//
// Created by td_main on 2023/7/5.
//

#pragma once

// clang-format off
#include <algorithm>
#include <atomic>
#include <cwchar>
#include <filesystem>
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

  void read_changes_async();
  void cancel();

  void on_sync_root_file_changes();

  inline FSys::path& get_child_path() { return child_path; }
  inline const FSys::path& get_child_path() const { return child_path; }

 private:
  void init();

  FSys::path child_path;
  std::unique_ptr<FILE_NOTIFY_INFORMATION> notify_;
  std::vector<std::tuple<DWORD, FSys::path>> files_;
  std::shared_ptr<boost::asio::windows::overlapped_ptr> overlapped_ptr_;
  boost::asio::any_io_executor executor_;
  wil::unique_hfile dir_;
};

}  // namespace doodle
