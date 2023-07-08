//
// Created by td_main on 2023/7/5.
//

#include "cloud_provider_registrar.h"

#include "doodle_core/core/global_function.h"
#include "doodle_core/logger/logger.h"
#include <doodle_core/core/file_sys.h>

#include "boost/asio/buffer.hpp"
#include "boost/asio/read.hpp"
#include "boost/asio/read_until.hpp"
#include "boost/asio/windows/overlapped_handle.hpp"
#include "boost/asio/windows/overlapped_ptr.hpp"
#include "boost/asio/windows/random_access_handle.hpp"
#include "boost/asio/windows/stream_handle.hpp"
#include "boost/exception/diagnostic_information.hpp"
#include "boost/numeric/conversion/cast.hpp"
#include "boost/system/system_error.hpp"
#include "boost/winapi/get_last_error.hpp"
#include <boost/asio.hpp>
// #include <doodle_core/lib_warp/
#include "fmt/ostream.h"
#include "magic_enum.hpp"
#include <Shlwapi.h>
#include <cstddef>
#include <cstdint>
#include <doodle_cloud_drive/cloud/detail/cloud_fetch_data.h>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>
#include <wil/result.h>
namespace doodle {

class cloud_fetch_placeholders : public std::enable_shared_from_this<cloud_fetch_placeholders> {
 public:
  explicit cloud_fetch_placeholders(boost::asio::io_context&, FSys::path in_server_path, FSys::path in_child_path, CF_CALLBACK_INFO in_callback_info_, const CF_CALLBACK_PARAMETERS*)
      : server_path_{std::move(in_server_path)},
        child_path_{std::move(in_child_path)},
        search_path_{reinterpret_cast<wchar_t const*>(in_callback_info_.FileIdentity)},
        create_placeholder_path_{FSys::path{in_callback_info_.VolumeDosName} / in_callback_info_.NormalizedPath},
        callback_info_{in_callback_info_} {}
  ~cloud_fetch_placeholders() = default;
  void async_run() {
    boost::asio::post(g_io_context(), [self = shared_from_this()]() { self->init(); });
  }

 private:
  struct data_value {
    std::wstring relative_file_name;
    std::wstring file_identity;
  };
  FSys::path server_path_;
  FSys::path child_path_;
  /// 服务器搜索路径
  FSys::path search_path_;
  /// 创建占位符的路径
  FSys::path create_placeholder_path_;
  /// 回调信息
  CF_CALLBACK_INFO callback_info_;
  std::vector<CF_PLACEHOLDER_CREATE_INFO> placeholder_create_infos_;
  std::vector<std::shared_ptr<data_value>> data_values_;

  NTSTATUS ntstatus_{STATUS_SUCCESS};

  std::size_t file_count_{0};
  std::size_t entries_processed_{0};
  CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAGS flags_{CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAG_NONE};

  void init() {
    file_count_ = std::distance(FSys::directory_iterator{search_path_}, FSys::directory_iterator{});

    server_path_.make_preferred();
    search_path_.make_preferred();
    child_path_.make_preferred();

    WIN32_FIND_DATA l_find_Data;
    HANDLE l_hfile_handle;

    auto l_search_path = search_path_.native() + L"\\*";

    l_hfile_handle =
        ::FindFirstFileExW(l_search_path.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0);
    if (l_hfile_handle == INVALID_HANDLE_VALUE) {
      ntstatus_ = NTSTATUS_FROM_WIN32(::GetLastError());
      return;
    }

    std::int32_t l_end{};

    do {
      do {
        if (l_find_Data.cFileName[0] == L'.' &&
            (l_find_Data.cFileName[1] == L'\0' || l_find_Data.cFileName[1] == L'.')) {
          continue;
        }
        std::error_code l_ec{};
        if (auto l_c_path = create_placeholder_path_ / l_find_Data.cFileName; FSys::exists(l_c_path, l_ec) || l_ec) {
          if (l_ec) {
            DOODLE_LOG_ERROR("error:{}", l_ec.message());
          }
          convert_to_placeholder(l_c_path);
          DOODLE_LOG_INFO("file exist:{}", l_c_path);
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

      } while (l_end = ::FindNextFileW(l_hfile_handle, &l_find_Data), (l_end && placeholder_create_infos_.size() < 20));
      if (!l_end) flags_ |= CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAG_DISABLE_ON_DEMAND_POPULATION;
      if (placeholder_create_infos_.empty()) {
        fail();
      }

      transfer_data(callback_info_.ConnectionKey, callback_info_.TransferKey);
      entries_processed_ += placeholder_create_infos_.size();
      placeholder_create_infos_.clear();
      data_values_.clear();
      //      l_end = ::FindNextFileW(l_hfile_handle, &l_find_Data);
    } while (l_end);

    ::FindClose(l_hfile_handle);
  }

  void convert_to_placeholder(const FSys::path& in_local_path) {
    WIN32_FIND_DATA l_find_Data;

    HANDLE l_hfile_handle =
        ::FindFirstFileExW(in_local_path.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0);
    if (l_hfile_handle == INVALID_HANDLE_VALUE) {
      ntstatus_ = NTSTATUS_FROM_WIN32(::GetLastError());
      return;
    }
    CF_PLACEHOLDER_STATE l_state = ::CfGetPlaceholderStateFromFindData(&l_find_Data);
    switch (l_state) {
      case CF_PLACEHOLDER_STATE_NO_STATES:
      case CF_PLACEHOLDER_STATE_PLACEHOLDER:
      case CF_PLACEHOLDER_STATE_SYNC_ROOT:
      case CF_PLACEHOLDER_STATE_ESSENTIAL_PROP_PRESENT:
      case CF_PLACEHOLDER_STATE_IN_SYNC:
      case CF_PLACEHOLDER_STATE_PARTIAL:
      case CF_PLACEHOLDER_STATE_PARTIALLY_ON_DISK: {
        HANDLE l_file_h{};
        LOG_IF_FAILED(::CfOpenFileWithOplock(in_local_path.c_str(), CF_OPEN_FILE_FLAG_EXCLUSIVE, &l_file_h));

        if (l_file_h == INVALID_HANDLE_VALUE) {
          return;
          LOG_LAST_ERROR();
        }
        CF_CONVERT_FLAGS l_flags =
            (l_state == CF_PLACEHOLDER_STATE_IN_SYNC ? CF_CONVERT_FLAG_MARK_IN_SYNC : CF_CONVERT_FLAG_NONE);

        if (FSys::is_directory(in_local_path)) {
          l_flags |= CF_CONVERT_FLAG_ENABLE_ON_DEMAND_POPULATION;
        }

        LOG_IF_FAILED(::CfConvertToPlaceholder(
            l_file_h, in_local_path.c_str(), in_local_path.native().size() * sizeof(wchar_t), l_flags, nullptr, nullptr
        ));
        ::CfCloseHandle(l_file_h);
        break;
      }
      default:
        DOODLE_LOG_INFO("Unknown state: {} {}", in_local_path, magic_enum::enum_name(l_state));
        break;
    }
  }

  void transfer_data(_In_ CF_CONNECTION_KEY connectionKey, _In_ LARGE_INTEGER transferKey) {
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
  // 失败
  void fail() {
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
};

namespace {
/**
 * @brief CF_CALLBACK_TYPE_FETCH_DATA 此回调用于向同步提供程序询问所需的一系列文件数据，以满足占位符上的 I/O
 * 请求或显式水合请求。 如果同步提供程序指定的水合策略在同步根注册时不是 ALWAYS_FULL，则需要实现此回调。
 * @param
 */
void CALLBACK
on_fetch_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
  auto* l_cloud_provider_registrar = reinterpret_cast<cloud_provider_registrar*>(callbackInfo->CallbackContext);

  FSys::path const l_server_path{reinterpret_cast<wchar_t const*>(callbackInfo->FileIdentity)};
  FSys::path const l_child_path = FSys::path{callbackInfo->VolumeDosName} / callbackInfo->NormalizedPath;
  //  {
  //    auto l_log = fmt::format(
  //        L"on_fetch_data: {} -> {} Received data request from {} for {}{}, priority {}, offset {}`{} length {}`{}",
  //        l_server_path.wstring(), l_child_path.wstring(),
  //        (callbackInfo->ProcessInfo && callbackInfo->ProcessInfo->ImagePath) ? callbackInfo->ProcessInfo->ImagePath
  //                                                                            : L"UNKNOWN",
  //        callbackInfo->VolumeDosName, callbackInfo->NormalizedPath, callbackInfo->PriorityHint,
  //        callbackParameters->FetchData.RequiredFileOffset.HighPart,
  //        callbackParameters->FetchData.RequiredFileOffset.LowPart,
  //        callbackParameters->FetchData.RequiredLength.HighPart, callbackParameters->FetchData.RequiredLength.LowPart
  //    );
  //    fmt::print(l_log);
  //  }

  try {
    auto l_ptr = std::make_shared<detail::cloud_fetch_data>(
        g_io_context(), l_server_path, l_child_path, *callbackInfo, callbackParameters
    );
    l_cloud_provider_registrar->cloud_fetch_data_list.emplace(callbackInfo->FileId.QuadPart, l_ptr);
    l_ptr->async_read();
  } catch (const boost::system::system_error& in_error) {
    DOODLE_LOG_INFO(boost::diagnostic_information(in_error));
  }
  CATCH_LOG()
}
/**
 * @brief CF_CALLBACK_TYPE_VALIDATE_DATA
 * 此回调用于请求同步提供程序确认给定范围的文件数据（之前的 CF_OPERATION_TYPE_TRANSFER_DATA
 * 操作已存在于磁盘上）是否有效， 因此平台可以使用它来满足用户 I/O
 * 请求。仅当同步提供程序在同步根注册时指定水合策略修饰符VALIDATION_REQUIRED时，才需要实现此回调。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK
on_validate_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {}
/**
 * @brief CF_CALLBACK_TYPE_CANCEL_FETCH_DATA
 * 此回调用于通知同步提供程序不再需要一系列文件数据，通常是因为原始请求已被取消。
 * 这允许同步提供程序停止花费精力尝试获取数据（取消未完成的网络请求等）。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK
on_cancel_fetch_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
  auto* l_cloud_provider_registrar = reinterpret_cast<cloud_provider_registrar*>(callbackInfo->CallbackContext);
  DOODLE_LOG_INFO(
      "取消请求 {} {}", callbackInfo->FileIdentity, magic_enum::enum_name(callbackParameters->Cancel.Flags)
  );
  boost::asio::post(g_io_context(), [=, l_id = callbackInfo->FileId.QuadPart]() {
    if (auto l_ptr = l_cloud_provider_registrar->cloud_fetch_data_list[l_id]; !l_ptr.expired()) {
      l_ptr.lock()->cancel();
    }
  });
}
/**
 * @brief CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS
 * 此回调用于要求同步提供程序提供有关占位符目录内容的信息，以满足目录查询操作或尝试打开目录下的文件。
 * 仅当同步提供程序在同步根注册时指定除CF_POPULATION_POLICY_ALWAYS_FULL之外的策略时，才需要实现此回调。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK on_fetch_placeholders(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {
  try {
    auto* l_cloud_provider_registrar = reinterpret_cast<cloud_provider_registrar*>(callbackInfo->CallbackContext);
    //  DOODLE_LOG_INFO("收到占位符请求 {} {}", callbackInfo->FileIdentity, callbackInfo->NormalizedPath);
    auto l_ptr                       = std::make_shared<cloud_fetch_placeholders>(
        g_io_context(), l_cloud_provider_registrar->server_path(), l_cloud_provider_registrar->child_path(),
        *callbackInfo, callbackParameters
    );
    l_ptr->async_run();
  }
  CATCH_LOG();
}
/**
 * @brief 此回调用于通知同步提供程序其同步根之一下的占位符已成功打开以进行读/写/删除访问。
 * 执行打开操作的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK on_notify_file_open_completion(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
/**
 * @brief CF_CALLBACK_TYPE_NOTIFY_FILE_CLOSE_COMPLETION
 * 此回调用于通知同步提供程序，先前为读/写/删除访问而打开的同步根之一下的占位符现在已关闭。
 * 执行关闭的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK on_notify_file_close_completion(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
/**
 * @brief CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE 此回调用于通知同步提供程序其同步根之一下的占位符即将脱水。
 * 执行脱水的用户应用程序被阻止。预计同步提供商会做出响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK
on_notify_dehydrate(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {}
/**
 * @brief CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE_COMPLETION 此回调用于通知同步提供程序其同步根之一下的占位符已成功脱水。
 * 执行脱水的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK on_notify_dehydrate_completion(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
/**
 * @brief CF_CALLBACK_TYPE_NOTIFY_DELETE 此回调用于通知同步提供程序其同步根之一下的占位符即将被删除。
 * 执行删除的用户应用程序被阻止。预计同步提供商会做出响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK
on_notify_delete(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {}
/**
 * @brief CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION
 * 此回调用于通知同步提供程序其同步根之一下的占位符已成功删除。执行删除的用户应用程序不会被阻止。
 * 预计同步提供商不会做出任何响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK on_notify_delete_completion(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
/**
 * CF_CALLBACK_TYPE_NOTIFY_RENAME 此回调用于通知同步提供程序其同步根之一下的占位符即将被重命名或移动。
 * 执行重命名/移动的用户应用程序被阻止。预计同步提供商会做出响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK
on_notify_rename(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {}
/**
 * @brief CF_CALLBACK_TYPE_NOTIFY_RENAME_COMPLETION
 * 此回调用于通知同步提供程序其同步根之一下的占位符已成功重命名或移动。
 * 执行重命名/移动的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
 * @param callbackInfo
 * @param callbackParameters
 */
void CALLBACK on_notify_rename_completion(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}

}  // namespace

void cloud_provider_registrar::init2() {
  root_        = root_.lexically_normal();
  server_root_ = server_path().lexically_normal();
  root_.make_preferred();
  server_root_.make_preferred();

  // 使用win32 api注册同步文件夹
  //    auto l_sync_root_id         = get_sync_root_id();
  GUID const l_guid          = {0xA3DDE735, 0xEDC1, 0x404D, {0x87, 0xA2, 0xA5, 0x06, 0xDB, 0x7B, 0x9C, 0x36}};
  CF_SYNC_REGISTRATION l_reg = {0};
  l_reg.StructSize           = sizeof(l_reg);
  l_reg.ProviderName         = L"Doodle.Sync";
  l_reg.ProviderVersion      = L"1.0";
  l_reg.ProviderId           = l_guid;
  CF_SYNC_POLICIES policies  = {0};
  policies.StructSize        = sizeof(policies);
  policies.HardLink          = CF_HARDLINK_POLICY_NONE;
  policies.InSync            = CF_INSYNC_POLICY_NONE;
  policies.Hydration.Primary = CF_HYDRATION_POLICY_PARTIAL;
  // CF_HYDRATION_POLICY_MODIFIER_VALIDATION_REQUIRED
  policies.Hydration.Modifier =
      CF_HYDRATION_POLICY_MODIFIER_AUTO_DEHYDRATION_ALLOWED | CF_HYDRATION_POLICY_MODIFIER_ALLOW_FULL_RESTART_HYDRATION;
  policies.Population.Primary  = CF_POPULATION_POLICY_PARTIAL;
  policies.Population.Modifier = CF_POPULATION_POLICY_MODIFIER_NONE;
  THROW_IF_FAILED(
      CfRegisterSyncRoot(root_.c_str(), &l_reg, &policies, CF_REGISTER_FLAG_DISABLE_ON_DEMAND_POPULATION_ON_ROOT)
  );
  static CF_CALLBACK_REGISTRATION s_MirrorCallbackTable[] = {
      {CF_CALLBACK_TYPE_FETCH_DATA, on_fetch_data},
      {CF_CALLBACK_TYPE_VALIDATE_DATA, on_validate_data},
      {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, on_cancel_fetch_data},
      {CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS, on_fetch_placeholders},
      {CF_CALLBACK_TYPE_NOTIFY_FILE_OPEN_COMPLETION, on_notify_file_open_completion},
      {CF_CALLBACK_TYPE_NOTIFY_FILE_CLOSE_COMPLETION, on_notify_file_close_completion},
      {CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE, on_notify_dehydrate},
      {CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE_COMPLETION, on_notify_dehydrate_completion},
      {CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION, on_notify_delete_completion},
      {CF_CALLBACK_TYPE_NOTIFY_DELETE, on_notify_delete},
      {CF_CALLBACK_TYPE_NOTIFY_RENAME, on_notify_rename},
      {CF_CALLBACK_TYPE_NOTIFY_RENAME_COMPLETION, on_notify_rename_completion},
      CF_CALLBACK_REGISTRATION_END};

  THROW_IF_FAILED(::CfConnectSyncRoot(
      root_.c_str(), s_MirrorCallbackTable, this,
      CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH, &s_transferCallbackConnectionKey
  ));
  //  auto l_file_h = ::CreateFileW(
  //      root_.c_str(), WRITE_DAC, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT |
  //      FILE_FLAG_BACKUP_SEMANTICS, nullptr
  //  );

  HANDLE l_file_h{};
  auto hr = ::CfOpenFileWithOplock(root_.c_str(), CF_OPEN_FILE_FLAG_EXCLUSIVE, &l_file_h);
  LOG_IF_FAILED(hr);

  if (hr != S_OK) {
    return;
    LOG_LAST_ERROR();
  }
  LOG_IF_FAILED(::CfConvertToPlaceholder(
      l_file_h, root_.c_str(), root_.native().size() * sizeof(wchar_t), CF_CONVERT_FLAG_MARK_IN_SYNC, nullptr, nullptr
  ));
  ::CfCloseHandle(l_file_h);
}

void cloud_provider_registrar::list_dir_info(const FSys::path& in_parent) {
  WIN32_FIND_DATA l_find_Data;
  HANDLE l_hfile_handle;

  auto l_search_path       = in_parent.native() + L"\\*";
  FSys::path l_parent_path = (root_ / in_parent.lexically_relative(server_root_)).make_preferred();
  if (l_parent_path.native().back() != L'\\') {
    l_parent_path += L"\\";
  }
  l_hfile_handle =
      ::FindFirstFileExW(l_search_path.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0);

  if (l_hfile_handle == INVALID_HANDLE_VALUE) {
    return;
  }

  do {
    if (l_find_Data.cFileName[0] == L'.' && (l_find_Data.cFileName[1] == L'\0' || l_find_Data.cFileName[1] == L'.')) {
      continue;
    }
    if (auto l_clo_path = (l_parent_path / l_find_Data.cFileName).lexically_normal(); FSys::exists(l_clo_path)) {
      l_clo_path.make_preferred();
      CF_PLACEHOLDER_STATE l_state = ::CfGetPlaceholderStateFromFindData(&l_find_Data);
      if (FSys::is_directory(l_clo_path)) {
        switch (l_state) {
          case CF_PLACEHOLDER_STATE_NO_STATES:
          case CF_PLACEHOLDER_STATE_PLACEHOLDER:
          case CF_PLACEHOLDER_STATE_SYNC_ROOT:
          case CF_PLACEHOLDER_STATE_ESSENTIAL_PROP_PRESENT:
          case CF_PLACEHOLDER_STATE_IN_SYNC:
          case CF_PLACEHOLDER_STATE_PARTIAL:
          case CF_PLACEHOLDER_STATE_PARTIALLY_ON_DISK: {
            HANDLE l_file_h{};
            LOG_IF_FAILED(::CfOpenFileWithOplock(l_clo_path.c_str(), CF_OPEN_FILE_FLAG_EXCLUSIVE, &l_file_h));

            if (l_file_h == INVALID_HANDLE_VALUE) {
              continue;
              LOG_LAST_ERROR();
            }
            CF_CONVERT_FLAGS l_flags =
                (l_state == CF_PLACEHOLDER_STATE_IN_SYNC ? CF_CONVERT_FLAG_MARK_IN_SYNC : CF_CONVERT_FLAG_NONE) |
                CF_CONVERT_FLAG_ENABLE_ON_DEMAND_POPULATION;

            LOG_IF_FAILED(::CfConvertToPlaceholder(
                l_file_h, l_clo_path.c_str(), l_clo_path.native().size() * sizeof(wchar_t), l_flags, nullptr, nullptr
            ));
            ::CfCloseHandle(l_file_h);
            break;
          }
          default:
            DOODLE_LOG_INFO("Unknown state: {}", magic_enum::enum_name(l_state));
            break;
        }
      }
    } else {
      CF_PLACEHOLDER_CREATE_INFO l_cloud_entry{};
      auto l_path                      = (in_parent / l_find_Data.cFileName).lexically_normal().make_preferred();
      l_cloud_entry.FileIdentity       = l_path.native().c_str();
      l_cloud_entry.FileIdentityLength = (l_path.native().size() + 1) * sizeof(wchar_t);

      l_cloud_entry.RelativeFileName   = l_find_Data.cFileName;
      l_cloud_entry.FsMetadata.FileSize.QuadPart =
          (boost::numeric_cast<ULONGLONG>(l_find_Data.nFileSizeHigh) << 32) + l_find_Data.nFileSizeLow;
      l_cloud_entry.FsMetadata.BasicInfo.FileAttributes = l_find_Data.dwFileAttributes;
      l_cloud_entry.FsMetadata.BasicInfo.CreationTime   = file_time_to_large_integer(l_find_Data.ftCreationTime);
      l_cloud_entry.FsMetadata.BasicInfo.LastAccessTime = file_time_to_large_integer(l_find_Data.ftLastAccessTime);
      l_cloud_entry.FsMetadata.BasicInfo.LastWriteTime  = file_time_to_large_integer(l_find_Data.ftLastWriteTime);
      l_cloud_entry.FsMetadata.BasicInfo.ChangeTime     = file_time_to_large_integer(l_find_Data.ftLastWriteTime);

      l_cloud_entry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC | CF_PLACEHOLDER_CREATE_FLAG_SUPERSEDE;
      LOG_IF_FAILED(::CfCreatePlaceholders(
          l_parent_path.native().c_str(), &l_cloud_entry, 1, CF_CREATE_FLAG_STOP_ON_ERROR, nullptr
      ));
    }

  } while (::FindNextFileW(l_hfile_handle, &l_find_Data));

  ::FindClose(l_hfile_handle);
}
void cloud_provider_registrar::create_placeholder(const FSys::path& in_parent) {
  auto l_path = in_parent.lexically_normal();
  l_path.make_preferred();
  list_dir_info(l_path);
}

void cloud_provider_registrar::uninit2() {
  LOG_IF_FAILED(CfDisconnectSyncRoot(s_transferCallbackConnectionKey));
  LOG_IF_FAILED(CfUnregisterSyncRoot(root_.generic_wstring().c_str()));
}
}  // namespace doodle