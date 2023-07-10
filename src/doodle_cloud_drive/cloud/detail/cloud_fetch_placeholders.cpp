//
// Created by td_main on 2023/7/8.
//

#include "cloud_fetch_placeholders.h"

#include "doodle_core/core/global_function.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/logger/logger.h>

#include "boost/asio/post.hpp"
#include <boost/asio.hpp>

#include "cloud_convert_to_placeholder.h"
#include <doodle_cloud_drive/cloud/cloud_provider_registrar.h>
#include <doodle_cloud_drive/cloud/detail/cloud_convert_to_placeholder.h>
#include <magic_enum.hpp>
#include <memory>
namespace doodle::detail {
using unique_cf_hfile = wil::unique_any_handle_invalid<decltype(&::CfCloseHandle), ::CfCloseHandle>;

void cloud_fetch_placeholders::async_run() {
  boost::asio::post(g_io_context(), [self = shared_from_this()]() { self->init(); });
}
void cloud_fetch_placeholders::init() {
  file_count_ = std::distance(FSys::directory_iterator{search_path_}, FSys::directory_iterator{});

  server_path_.make_preferred();
  search_path_.make_preferred();
  child_path_.make_preferred();

  WIN32_FIND_DATA l_find_Data;

  auto l_search_path = search_path_.native() + L"\\*";
  wil::unique_hfind l_hfind{
      ::FindFirstFileExW(l_search_path.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0)};

  if (!l_hfind.is_valid()) {
    ntstatus_ = NTSTATUS_FROM_WIN32(::GetLastError());
    return;
  }

  std::int32_t l_end{};

  do {
    do {
      if (l_find_Data.cFileName[0] == L'.' && (l_find_Data.cFileName[1] == L'\0' || l_find_Data.cFileName[1] == L'.')) {
        continue;
      }
      std::error_code l_ec{};
      if (auto l_c_path = create_placeholder_path_ / l_find_Data.cFileName; FSys::exists(l_c_path, l_ec) || l_ec) {
        if (l_ec) {
          DOODLE_LOG_ERROR("error:{}", l_ec.message());
        }
        DOODLE_LOG_INFO("file exist:{}", l_c_path);
        auto l_conv = std::make_shared<cloud_convert_to_placeholder>(
            g_io_context(), l_c_path, search_path_ / l_find_Data.cFileName
        );
        l_conv->async_run();
      } else {
        CF_PLACEHOLDER_CREATE_INFO& l_cloud_entry = placeholder_create_infos_.emplace_back();
        auto& l_data_value                        = data_values_.emplace_back(std::make_shared<data_value>());
        l_data_value->relative_file_name          = l_find_Data.cFileName;
        l_data_value->file_identity               = (search_path_ / l_find_Data.cFileName).native();

        l_cloud_entry.FileIdentity                = l_data_value->file_identity.c_str();
        l_cloud_entry.FileIdentityLength = static_cast<ULONG>(l_data_value->file_identity.size() * sizeof(wchar_t));
        l_cloud_entry.RelativeFileName   = l_data_value->relative_file_name.c_str();
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
      }

    } while (l_end = ::FindNextFileW(l_hfind.get(), &l_find_Data), (l_end && placeholder_create_infos_.size() < 20));
    if (!l_end) flags_ |= CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAG_DISABLE_ON_DEMAND_POPULATION;
    if (placeholder_create_infos_.empty()) {
      fail();
    }

    transfer_data(callback_info_.ConnectionKey, callback_info_.TransferKey);
    entries_processed_ += placeholder_create_infos_.size();
    placeholder_create_infos_.clear();
    data_values_.clear();
  } while (l_end);
}
void cloud_fetch_placeholders::transfer_data(CF_CONNECTION_KEY connectionKey, LARGE_INTEGER transferKey) {
  DOODLE_LOG_INFO("开始批量创建标识符 {}", search_path_);
  CF_OPERATION_INFO opInfo                       = {0};
  CF_OPERATION_PARAMETERS opParams               = {0};

  opInfo.StructSize                              = sizeof(opInfo);
  opInfo.Type                                    = CF_OPERATION_TYPE_TRANSFER_PLACEHOLDERS;
  opInfo.ConnectionKey                           = connectionKey;
  opInfo.TransferKey                             = transferKey;
  opInfo.RequestKey                              = callback_info_.RequestKey;

  opParams.ParamSize                             = sizeof(opParams);

  opParams.TransferPlaceholders.Flags            = flags_;
  opParams.TransferPlaceholders.CompletionStatus = ntstatus_;
  opParams.TransferPlaceholders.PlaceholderTotalCount =
      cloud_provider_registrar::longlong_to_large_integer(file_count_);
  opParams.TransferPlaceholders.PlaceholderArray = placeholder_create_infos_.data();
  opParams.TransferPlaceholders.PlaceholderCount = placeholder_create_infos_.size();
  opParams.TransferPlaceholders.EntriesProcessed = entries_processed_;

  LOG_IF_FAILED(::CfExecute(&opInfo, &opParams));
}
void cloud_fetch_placeholders::fail() {
  CF_OPERATION_INFO opInfo                       = {0};
  CF_OPERATION_PARAMETERS opParams               = {0};

  opInfo.StructSize                              = sizeof(opInfo);
  opInfo.Type                                    = CF_OPERATION_TYPE_TRANSFER_PLACEHOLDERS;
  opInfo.ConnectionKey                           = callback_info_.ConnectionKey;
  opInfo.TransferKey                             = callback_info_.TransferKey;
  opInfo.RequestKey                              = callback_info_.RequestKey;

  opParams.ParamSize                             = sizeof(opParams);

  opParams.TransferPlaceholders.Flags            = flags_;
  opParams.TransferPlaceholders.CompletionStatus = ntstatus_;
  opParams.TransferPlaceholders.PlaceholderArray = nullptr;
  opParams.TransferPlaceholders.PlaceholderCount = 0;
  opParams.TransferPlaceholders.EntriesProcessed = 0;

  LOG_IF_FAILED(::CfExecute(&opInfo, &opParams));
}
}  // namespace doodle::detail