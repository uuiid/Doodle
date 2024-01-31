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
#include <Windows.h>
#include "wil/result.h"
#include <sddl.h>
#include <Unknwn.h>
#include <winternl.h>
#include <comutil.h>
#include <oleidl.h>
#include <ntstatus.h>
#include <cfapi.h>
// clang-format on
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/logger/logger.h"

#include "magic_enum.hpp"
#include <doodle_cloud_drive/cloud/cloud_provider_registrar.h>
namespace doodle::detail {
using unique_cf_hfile = wil::unique_any_handle_invalid<decltype(&::CfCloseHandle), ::CfCloseHandle>;

void cloud_convert_to_placeholder::async_run() {
  std::error_code l_ec{};
  if (FSys::is_directory(child_path_, l_ec) && !sub_run) {
    boost::asio::post(executor_, [self = shared_from_this()]() { self->init_child_placeholder(); });
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
  if (!FSys::exists(server_path_)) return;

  //  WIN32_FIND_DATA l_find_Data;

  //  wil::unique_hfind l_hfind{
  //      ::FindFirstFileExW(child_path_.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0)};
  //
  //  if (!l_hfind.is_valid()) {
  //    ntstatus_ = NTSTATUS_FROM_WIN32(::GetLastError());
  //    return;
  //  }
  std::error_code l_ec{};
  const auto l_is_dir  = FSys::is_directory(child_path_, l_ec);
  const auto l_is_file = FSys::is_regular_file(child_path_, l_ec);

  if (l_is_dir) {
    const CF_CONVERT_FLAGS l_flags = CF_CONVERT_FLAG_NONE | CF_CONVERT_FLAG_ENABLE_ON_DEMAND_POPULATION;
    unique_cf_hfile l_file_h{};
    if (FAILED_LOG(::CfOpenFileWithOplock(child_path_.c_str(), CF_OPEN_FILE_FLAG_FOREGROUND, l_file_h.put())))
      DOODLE_LOG_INFO("async_convert_to_placeholder:{} {}", child_path_, server_path_);

    if (l_file_h.is_valid()) {
      if (FAILED_LOG(::CfConvertToPlaceholder(
              l_file_h.get(), server_path_.c_str(), server_path_.native().size() * sizeof(wchar_t), l_flags, nullptr,
              nullptr
          )))
        DOODLE_LOG_INFO("async_convert_to_placeholder:{} {}", child_path_, server_path_);
    }
  } else if (l_is_file) {
    WIN32_FIND_DATA l_find_Data;
    const wil::unique_hfind l_find_handle{
        ::FindFirstFileExW(server_path_.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0)
    };

    if (!l_find_handle.is_valid()) {
      return;
    }
    CF_PLACEHOLDER_CREATE_INFO l_cloud_entry{};
    l_cloud_entry.FileIdentity       = server_path_.c_str();
    l_cloud_entry.FileIdentityLength = (server_path_.native().size()) * sizeof(wchar_t);
    auto l_file_name                 = child_path_.filename();
    auto l_parent_path               = child_path_.parent_path();
    l_cloud_entry.RelativeFileName   = l_file_name.native().c_str();
    l_cloud_entry.FsMetadata.FileSize.QuadPart =
        (boost::numeric_cast<ULONGLONG>(l_find_Data.nFileSizeHigh) << 32) + l_find_Data.nFileSizeLow;
    l_cloud_entry.FsMetadata.BasicInfo.FileAttributes = l_find_Data.dwFileAttributes;
    l_cloud_entry.FsMetadata.BasicInfo.CreationTime =
        cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftCreationTime);
    l_cloud_entry.FsMetadata.BasicInfo.LastAccessTime =
        cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftLastAccessTime);
    l_cloud_entry.FsMetadata.BasicInfo.LastWriteTime =
        cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftLastWriteTime);
    l_cloud_entry.FsMetadata.BasicInfo.ChangeTime =
        cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftLastWriteTime);

    l_cloud_entry.Flags = CF_PLACEHOLDER_CREATE_FLAG_SUPERSEDE;
    if (FAILED_LOG(::CfCreatePlaceholders(
            l_parent_path.native().c_str(), &l_cloud_entry, 1, CF_CREATE_FLAG_STOP_ON_ERROR, nullptr
        )))
      DOODLE_LOG_INFO("async_convert_to_placeholder:{} {}", child_path_, server_path_);
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
}
void cloud_convert_to_placeholder::init_child_placeholder() {
  WIN32_FIND_DATA l_find_Data;

  auto l_search_path       = server_path_.native() + L"\\*";
  FSys::path l_parent_path = child_path_.make_preferred();
  if (l_parent_path.native().back() != L'\\') {
    l_parent_path += L"\\";
  }
  wil::unique_hfind l_find_handle{
      ::FindFirstFileExW(l_search_path.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0)
  };

  if (!l_find_handle.is_valid()) {
    return;
  }

  do {
    if (l_find_Data.cFileName[0] == L'.' && (l_find_Data.cFileName[1] == L'\0' || l_find_Data.cFileName[1] == L'.')) {
      continue;
    }
    std::error_code l_ec{};
    if (auto l_clo_path = (l_parent_path / l_find_Data.cFileName).lexically_normal();
        !FSys::exists(l_clo_path, l_ec) && !l_ec) {
      CF_PLACEHOLDER_CREATE_INFO l_cloud_entry{};
      auto l_path                      = (server_path_ / l_find_Data.cFileName).lexically_normal().make_preferred();
      l_cloud_entry.FileIdentity       = l_path.native().c_str();
      l_cloud_entry.FileIdentityLength = (l_path.native().size() + 1) * sizeof(wchar_t);

      l_cloud_entry.RelativeFileName   = l_find_Data.cFileName;
      l_cloud_entry.FsMetadata.FileSize.QuadPart =
          (boost::numeric_cast<ULONGLONG>(l_find_Data.nFileSizeHigh) << 32) + l_find_Data.nFileSizeLow;
      l_cloud_entry.FsMetadata.BasicInfo.FileAttributes = l_find_Data.dwFileAttributes;
      l_cloud_entry.FsMetadata.BasicInfo.CreationTime =
          cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftCreationTime);
      l_cloud_entry.FsMetadata.BasicInfo.LastAccessTime =
          cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftLastAccessTime);
      l_cloud_entry.FsMetadata.BasicInfo.LastWriteTime =
          cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftLastWriteTime);
      l_cloud_entry.FsMetadata.BasicInfo.ChangeTime =
          cloud_provider_registrar::file_time_to_large_integer(l_find_Data.ftLastWriteTime);

      l_cloud_entry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC | CF_PLACEHOLDER_CREATE_FLAG_SUPERSEDE;
      LOG_IF_FAILED(::CfCreatePlaceholders(
          l_parent_path.native().c_str(), &l_cloud_entry, 1, CF_CREATE_FLAG_STOP_ON_ERROR, nullptr
      ));
    }

  } while (::FindNextFileW(l_find_handle.get(), &l_find_Data));
}
// namespace doodle::detail

}  // namespace doodle::detail