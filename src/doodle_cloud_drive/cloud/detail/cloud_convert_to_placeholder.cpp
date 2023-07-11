//
// Created by td_main on 2023/7/10.
//

#include "cloud_convert_to_placeholder.h"

#include "boost/asio/post.hpp"
#include "boost/exception/diagnostic_information.hpp"
#include "boost/winapi/file_management.hpp"
// clang-format off
#include "range/v3/action/sort.hpp"
#include "wil/result_macros.h"
#include <filesystem>
#include <memory>
#include <system_error>
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

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include <magic_enum.hpp>
namespace doodle::detail {
using unique_cf_hfile = wil::unique_any_handle_invalid<decltype(&::CfCloseHandle), ::CfCloseHandle>;

void cloud_convert_to_placeholder::async_run() {
  std::error_code l_ec{};
  if (FSys::is_directory(child_path_, l_ec) && !sub_run) {
    std::vector<sub_path> l_file_list{};
    std::error_code l_ec{};
    for (auto l_it = FSys::recursive_directory_iterator{child_path_}; l_it != FSys::recursive_directory_iterator{};
         l_it.increment(l_ec)) {
      if (l_ec) DOODLE_LOG_INFO(l_ec.message());
      l_file_list.emplace_back(sub_path{l_it.depth(), l_it->path()});
    }
    l_file_list |= ranges::actions::sort([](const sub_path& l_left, const sub_path& l_right) {
      return l_left.deep_ > l_right.deep_;
    });
    for (auto& l_item : l_file_list) {
      auto l_convert = std::make_shared<cloud_convert_to_placeholder>(
          executor_, server_path_ / l_item.path_.lexically_relative(child_path_).make_preferred(), l_item.path_
      );
      l_convert->sub_run = true;
      l_convert->async_run();
    }
  }
  if (l_ec) DOODLE_LOG_INFO(l_ec.message());
  boost::asio::post(executor_, [self = shared_from_this()]() { self->async_convert_to_placeholder(); });
}
void cloud_convert_to_placeholder::async_convert_to_placeholder() {
  DOODLE_LOG_INFO("async_convert_to_placeholder:{} {}", child_path_, server_path_);

  //  WIN32_FIND_DATA l_find_Data;

  //  wil::unique_hfind l_hfind{
  //      ::FindFirstFileExW(child_path_.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0)};
  //
  //  if (!l_hfind.is_valid()) {
  //    ntstatus_ = NTSTATUS_FROM_WIN32(::GetLastError());
  //    return;
  //  }
  std::error_code l_ec{};
  auto l_is_dir            = FSys::is_directory(child_path_, l_ec);

  CF_CONVERT_FLAGS l_flags = CF_CONVERT_FLAG_NONE;
  //          (l_state == CF_PLACEHOLDER_STATE_IN_SYNC ? CF_CONVERT_FLAG_MARK_IN_SYNC : CF_CONVERT_FLAG_NONE);

  if (l_is_dir) {
    l_flags |= CF_CONVERT_FLAG_ENABLE_ON_DEMAND_POPULATION;
  } else if (FSys::is_regular_file(child_path_, l_ec) || l_ec) {
    l_flags |= CF_CONVERT_FLAG_DEHYDRATE;
  }

  //  wil::unique_hfile l_hfile{::CreateFileW(
  //      child_path_.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr
  //  )};
  unique_cf_hfile l_file_h{};
  LOG_IF_FAILED(::CfOpenFileWithOplock(
      child_path_.c_str(), CF_OPEN_FILE_FLAG_EXCLUSIVE | CF_OPEN_FILE_FLAG_WRITE_ACCESS, l_file_h.put()
  ));
  if (l_file_h.is_valid()) {
    DWORD l_size{};
    //    LOG_IF_FAILED(::CfGetPlaceholderInfo(l_file_h.get(), CF_PLACEHOLDER_INFO_STANDARD, nullptr, 0, &l_size));
    auto l_buff = std::make_unique<char[]>(sizeof(CF_PLACEHOLDER_STANDARD_INFO));
    auto l_h    = LOG_IF_FAILED(
        ::CfGetPlaceholderInfo(l_file_h.get(), CF_PLACEHOLDER_INFO_STANDARD, l_buff.get(), l_size, &l_size)
    );
    if (l_h != S_OK) {
      //          l_is_dir ? (CF_OPEN_FILE_FLAG_FOREGROUND) : (CF_OPEN_FILE_FLAG_EXCLUSIVE |
      //          CF_OPEN_FILE_FLAG_WRITE_ACCESS),
      LOG_IF_FAILED(::CfConvertToPlaceholder(
          l_file_h.get(), server_path_.c_str(), server_path_.native().size() * sizeof(wchar_t), l_flags, nullptr,
          nullptr
      ));
    }
  }

  //  break;

  //  CF_PLACEHOLDER_STATE l_state = ::CfGetPlaceholderStateFromFindData(&l_find_Data);
  //  switch (l_state) {
  //    case CF_PLACEHOLDER_STATE_NO_STATES:
  //    case CF_PLACEHOLDER_STATE_PLACEHOLDER:
  //    case CF_PLACEHOLDER_STATE_SYNC_ROOT:
  //    case CF_PLACEHOLDER_STATE_ESSENTIAL_PROP_PRESENT:
  //    case CF_PLACEHOLDER_STATE_IN_SYNC:
  //    case CF_PLACEHOLDER_STATE_PARTIAL:
  //    case CF_PLACEHOLDER_STATE_PARTIALLY_ON_DISK:
  //    case CF_PLACEHOLDER_STATE_INVALID:
  //
  //    {
  //
  //    }
  //    default:
  //      DOODLE_LOG_INFO("Unknown state: {} {}", child_path_, magic_enum::enum_name(l_state));
  //      break;
  //  }
}  // namespace doodle::detail

}  // namespace doodle::detail