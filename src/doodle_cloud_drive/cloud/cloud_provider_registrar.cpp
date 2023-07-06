//
// Created by td_main on 2023/7/5.
//

#include "cloud_provider_registrar.h"

#include "doodle_core/logger/logger.h"
#include <doodle_core/core/file_sys.h>

#include <boost/asio.hpp>
// #include <doodle_core/lib_warp/
#include <filesystem>
namespace doodle {

namespace {
/**
 * @brief CF_CALLBACK_TYPE_FETCH_DATA 此回调用于向同步提供程序询问所需的一系列文件数据，以满足占位符上的 I/O
 * 请求或显式水合请求。 如果同步提供程序指定的水合策略在同步根注册时不是 ALWAYS_FULL，则需要实现此回调。
 * @param
 */
void CALLBACK
on_fetch_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters) {
  auto* l_cloud_provider_registrar = reinterpret_cast<cloud_provider_registrar*>(callbackInfo->CallbackContext);
  FSys::path l_server_path{reinterpret_cast<wchar_t const*>(callbackInfo->FileIdentity)};
  FSys::path l_child_path = FSys::path{callbackInfo->VolumeDosName} / callbackInfo->NormalizedPath;
  callbackParameters->FetchData.RequiredFileOffset;
  {
    auto l_log = fmt::format(
        L"on_fetch_data: {} -> {} Received data request from {} for {}{}, priority {}, offset {}`{} length {}`{}",
        l_server_path.wstring(), l_child_path.wstring(),
        (callbackInfo->ProcessInfo && callbackInfo->ProcessInfo->ImagePath) ? callbackInfo->ProcessInfo->ImagePath
                                                                            : L"UNKNOWN",
        callbackInfo->VolumeDosName, callbackInfo->NormalizedPath, callbackInfo->PriorityHint,
        callbackParameters->FetchData.RequiredFileOffset.HighPart,
        callbackParameters->FetchData.RequiredFileOffset.LowPart, callbackParameters->FetchData.RequiredLength.HighPart,
        callbackParameters->FetchData.RequiredLength.LowPart
    );
    fmt::print(l_log);
  }

  boost::asio::windows::random_access_handle l_file_handle{g_io_context()};

  auto* l_file = ::CreateFileW(
      l_server_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr
  );
  if (l_file == INVALID_HANDLE_VALUE) {
    LOG_LAST_ERROR();
    auto l_log = fmt::format(
        L"on_fetch_data: {} -> {} CreateFileW failed with {}", l_server_path.wstring(), l_child_path.wstring(),
        ::GetLastError()
    );
    fmt::print(l_log);
    return;
  }

  auto l_buffer_size = std::min(callbackParameters->FetchData.RequiredLength.QuadPart, 4096ll);
}
/**
 * @brief CF_CALLBACK_TYPE_VALIDATE_DATA
 * 此回调用于请求同步提供程序确认给定范围的文件数据（之前的CF_OPERATION_TYPE_TRANSFER_DATA操作已存在于磁盘上）是否有效，
 * 因此平台可以使用它来满足用户 I/O
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
) {}
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
  // 使用win32 api注册同步文件夹
  //    auto l_sync_root_id         = get_sync_root_id();
  GUID const l_guid           = {0xA3DDE735, 0xEDC1, 0x404D, {0x87, 0xA2, 0xA5, 0x06, 0xDB, 0x7B, 0x9C, 0x36}};
  CF_SYNC_REGISTRATION l_reg  = {0};
  l_reg.StructSize            = sizeof(l_reg);
  l_reg.ProviderName          = L"Doodle.Sync";
  l_reg.ProviderVersion       = L"1.0";
  l_reg.ProviderId            = l_guid;
  CF_SYNC_POLICIES policies   = {0};
  policies.StructSize         = sizeof(policies);
  policies.HardLink           = CF_HARDLINK_POLICY_NONE;
  policies.InSync             = CF_INSYNC_POLICY_NONE;
  policies.Hydration.Primary  = CF_HYDRATION_POLICY_PARTIAL;
  policies.Hydration.Modifier = CF_HYDRATION_POLICY_MODIFIER_VALIDATION_REQUIRED |
                                CF_HYDRATION_POLICY_MODIFIER_AUTO_DEHYDRATION_ALLOWED |
                                CF_HYDRATION_POLICY_MODIFIER_ALLOW_FULL_RESTART_HYDRATION;
  policies.Population.Primary  = CF_POPULATION_POLICY_PARTIAL;
  policies.Population.Modifier = CF_POPULATION_POLICY_MODIFIER_NONE;
  THROW_IF_FAILED(CfRegisterSyncRoot(
      root_.generic_wstring().c_str(), &l_reg, &policies, CF_REGISTER_FLAG_DISABLE_ON_DEMAND_POPULATION_ON_ROOT
  ));
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
      root_.generic_wstring().c_str(), s_MirrorCallbackTable, this,
      CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH, &s_transferCallbackConnectionKey
  ));
}

void cloud_provider_registrar::list_dir_info(const FSys::path& in_parent) {
  WIN32_FIND_DATA l_find_Data;
  HANDLE l_hfile_handle;
  auto l_search_path = in_parent.generic_wstring() + L"\\*";
  auto l_parent_path = root_ / in_parent.lexically_relative(server_root_);
  l_hfile_handle =
      ::FindFirstFileExW(l_search_path.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0);

  if (l_hfile_handle == INVALID_HANDLE_VALUE) {
    return;
  }

  do {
    if (l_find_Data.cFileName[0] == L'.' && (l_find_Data.cFileName[1] == L'\0' || l_find_Data.cFileName[1] == L'.')) {
      continue;
    }
    CF_PLACEHOLDER_CREATE_INFO l_cloud_entry{};
    auto l_path                      = (in_parent / l_find_Data.cFileName).generic_wstring();
    l_cloud_entry.FileIdentity       = l_path.c_str();
    l_cloud_entry.FileIdentityLength = (l_path.size() + 1) * sizeof(wchar_t);

    l_cloud_entry.RelativeFileName   = l_find_Data.cFileName;
    l_cloud_entry.FsMetadata.FileSize.QuadPart =
        (boost::numeric_cast<ULONGLONG>(l_find_Data.nFileSizeHigh) << 32) + l_find_Data.nFileSizeLow;
    l_cloud_entry.FsMetadata.BasicInfo.FileAttributes = l_find_Data.dwFileAttributes;
    l_cloud_entry.FsMetadata.BasicInfo.CreationTime   = file_time_to_large_integer(l_find_Data.ftCreationTime);
    l_cloud_entry.FsMetadata.BasicInfo.LastAccessTime = file_time_to_large_integer(l_find_Data.ftLastAccessTime);
    l_cloud_entry.FsMetadata.BasicInfo.LastWriteTime  = file_time_to_large_integer(l_find_Data.ftLastWriteTime);
    l_cloud_entry.FsMetadata.BasicInfo.ChangeTime     = file_time_to_large_integer(l_find_Data.ftLastWriteTime);

    l_cloud_entry.Flags = CF_PLACEHOLDER_CREATE_FLAG_MARK_IN_SYNC | CF_PLACEHOLDER_CREATE_FLAG_SUPERSEDE;
    THROW_IF_FAILED(::CfCreatePlaceholders(
        l_parent_path.generic_wstring().c_str(), &l_cloud_entry, 1, CF_CREATE_FLAG_STOP_ON_ERROR, nullptr
    ));
  } while (::FindNextFileW(l_hfile_handle, &l_find_Data));

  ::FindClose(l_hfile_handle);
}
void cloud_provider_registrar::create_placeholder(const FSys::path& in_parent) { list_dir_info(in_parent); }

void cloud_provider_registrar::uninit2() {
  THROW_IF_FAILED(CfDisconnectSyncRoot(s_transferCallbackConnectionKey));
  THROW_IF_FAILED(CfUnregisterSyncRoot(root_.generic_wstring().c_str()));
}
}  // namespace doodle