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
#include <boost/asio.hpp>
// #include <doodle_core/lib_warp/
#include "fmt/ostream.h"
#include "magic_enum.hpp"
#include <Shlwapi.h>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace doodle {

class cloud_fetch_data : std::enable_shared_from_this<cloud_provider_registrar> {
  constexpr static std::size_t buffer_size = 4096;

 public:
  struct arg {
    OVERLAPPED Overlapped;
    CF_CALLBACK_INFO CallbackInfo;
    CHAR PriorityHint;
    LARGE_INTEGER StartOffset;
    LARGE_INTEGER RemainingLength;
    ULONG BufferSize;
  };

  explicit cloud_fetch_data(
      boost::asio::io_context& io_context, FSys::path in_server_path, FSys::path in_child_path,
      CF_CALLBACK_INFO in_callback_info_, const CF_CALLBACK_PARAMETERS* in_callback_parameters
  )
      : stream_handle_{io_context},
        server_path_{std::move(in_server_path)},
        child_path_{std::move(in_child_path)},
        length_{std::min(
            boost::numeric_cast<std::size_t>(in_callback_parameters->FetchData.RequiredLength.QuadPart), buffer_size
        )},
        buffer_{std::make_unique<char[]>(length_)},
        callback_info_{in_callback_info_},
        start_offset_{boost::numeric_cast<std::size_t>(in_callback_parameters->FetchData.RequiredFileOffset.QuadPart)},
        remaining_length_{in_callback_parameters->FetchData.RequiredLength} {
    init();
  }

 private:
  boost::asio::windows::random_access_handle stream_handle_;
  FSys::path server_path_;
  FSys::path child_path_;
  std::size_t length_;
  std::unique_ptr<char[]> buffer_;
  CF_CALLBACK_INFO callback_info_;
  std::size_t start_offset_{};
  LARGE_INTEGER remaining_length_{};

  void init() {
    stream_handle_.assign(::CreateFileW(
        server_path_.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, nullptr
    ));
  }

 public:
  void async_read() {
    boost::asio::async_read_at(
        stream_handle_, start_offset_, boost::asio::buffer(buffer_.get(), length_),
        [this, l_s = shared_from_this()](const boost::system::error_code& ec, std::size_t bytes_transferred) {
          if (ec && ec != boost::asio::error::eof) {
            // 读取失败
            DOODLE_LOG_INFO(
                "[{}:{}] - Async read failed for {}, Status {}\n", GetCurrentProcessId(), GetCurrentThreadId(),
                server_path_, ec.what()
            );
            return;
          }
          // 读取成功
          fmt::print(
              "[{}:{}] - Async read failed for {}, Status {}\n", GetCurrentProcessId(), GetCurrentThreadId(),
              server_path_, ec.what()
          );
          transfer_data(
              callback_info_.ConnectionKey, callback_info_.TransferKey, buffer_.get(), start_offset_, bytes_transferred,
              STATUS_SUCCESS
          );
          STATUS_CLOUD_FILE_IN_USE;
          start_offset_ += bytes_transferred;
          remaining_length_.QuadPart -= bytes_transferred;
          if (remaining_length_.QuadPart > 0) {
            async_read();
          }
        }
    );
  }

  void cancel() { stream_handle_.cancel(); }

  static void transfer_data(
      _In_ CF_CONNECTION_KEY connectionKey, _In_ LARGE_INTEGER transferKey,
      _In_reads_bytes_opt_(length.QuadPart) LPCVOID transferData, _In_ std::size_t startingOffset,
      _In_ std::size_t length, _In_ NTSTATUS completionStatus
  ) {
    CF_OPERATION_INFO opInfo               = {0};
    CF_OPERATION_PARAMETERS opParams       = {0};

    opInfo.StructSize                      = sizeof(opInfo);
    opInfo.Type                            = CF_OPERATION_TYPE_TRANSFER_DATA;
    opInfo.ConnectionKey                   = connectionKey;
    opInfo.TransferKey                     = transferKey;
    opParams.ParamSize                     = RTL_SIZEOF_THROUGH_FIELD(CF_OPERATION_PARAMETERS, TransferData);
    opParams.TransferData.CompletionStatus = completionStatus;
    opParams.TransferData.Flags            = CF_OPERATION_TRANSFER_DATA_FLAG_NONE;
    opParams.TransferData.Buffer           = transferData;
    opParams.TransferData.Offset           = cloud_provider_registrar::longlong_to_large_integer(startingOffset);
    opParams.TransferData.Length           = cloud_provider_registrar::longlong_to_large_integer(length);

    THROW_IF_FAILED(::CfExecute(&opInfo, &opParams));
  }
};

struct cf_placeholder_create_info : ::CF_PLACEHOLDER_CREATE_INFO {
 public:
  cf_placeholder_create_info()                                          = default;
  // copy constructor
  cf_placeholder_create_info(const cf_placeholder_create_info& in_info) = delete;
  //      : ::CF_PLACEHOLDER_CREATE_INFO(in_info) {
  //    RelativeFileName = new wchar_t[wcslen(in_info.RelativeFileName) + 1];
  //    wcscpy_s(const_cast<wchar_t*>(RelativeFileName), wcslen(in_info.RelativeFileName), in_info.RelativeFileName);
  //    FsMetadata = in_info.FsMetadata;
  //    auto* l_p  = malloc(in_info.FileIdentityLength);
  //    ::CopyMemory(l_p, in_info.FileIdentity, in_info.FileIdentityLength);
  //    FileIdentity       = l_p;
  //    FileIdentityLength = in_info.FileIdentityLength;
  //    Flags              = in_info.Flags;
  //    Result             = in_info.Result;
  //    CreateUsn          = in_info.CreateUsn;
  //  }
  // move constructor
  cf_placeholder_create_info(cf_placeholder_create_info&& in_info) noexcept {
    RelativeFileName           = in_info.RelativeFileName;
    FsMetadata                 = in_info.FsMetadata;
    FileIdentity               = in_info.FileIdentity;
    FileIdentityLength         = in_info.FileIdentityLength;
    Flags                      = in_info.Flags;
    Result                     = in_info.Result;
    CreateUsn                  = in_info.CreateUsn;

    in_info.RelativeFileName   = nullptr;
    in_info.FsMetadata         = {};
    in_info.FileIdentityLength = 0;
    in_info.Flags              = CF_PLACEHOLDER_CREATE_FLAGS::CF_PLACEHOLDER_CREATE_FLAG_NONE;
    in_info.Result             = 0;
    in_info.CreateUsn          = 0;
  }
  // copy assignment
  cf_placeholder_create_info& operator=(const cf_placeholder_create_info& in_info) = delete;
  //  {
  //    if (this == &in_info) {
  //      return *this;
  //    }
  //    RelativeFileName = new wchar_t[wcslen(in_info.RelativeFileName) + 1];
  //    wcscpy_s(const_cast<wchar_t*>(RelativeFileName), wcslen(in_info.RelativeFileName), in_info.RelativeFileName);
  //    FsMetadata = in_info.FsMetadata;
  //    auto* l_p  = ::LocalAlloc(LPTR, in_info.FileIdentityLength);
  //    ::CopyMemory(l_p, in_info.FileIdentity, in_info.FileIdentityLength);
  //    FileIdentity       = l_p;
  //    FileIdentityLength = in_info.FileIdentityLength;
  //    Flags              = in_info.Flags;
  //    Result             = in_info.Result;
  //    CreateUsn          = in_info.CreateUsn;
  //    return *this;
  //  }
  // move assignment
  cf_placeholder_create_info& operator=(cf_placeholder_create_info&& in_info) noexcept {
    if (this == &in_info) {
      return *this;
    }
    RelativeFileName           = in_info.RelativeFileName;
    FsMetadata                 = in_info.FsMetadata;
    FileIdentity               = in_info.FileIdentity;
    FileIdentityLength         = in_info.FileIdentityLength;
    Flags                      = in_info.Flags;
    Result                     = in_info.Result;
    CreateUsn                  = in_info.CreateUsn;

    in_info.RelativeFileName   = nullptr;
    in_info.FsMetadata         = {};
    in_info.FileIdentityLength = 0;
    in_info.Flags              = CF_PLACEHOLDER_CREATE_FLAGS::CF_PLACEHOLDER_CREATE_FLAG_NONE;
    in_info.Result             = 0;
    in_info.CreateUsn          = 0;
    return *this;
  }

  //  void set_file_identity(_In_ CONST FSys::path& in_path) {
  //    FileIdentityLength = in_path.native().size() * sizeof(wchar_t);
  //    FileIdentity       = ::LocalAlloc(LPTR, FileIdentityLength);
  //    ::CopyMemory(const_cast<void*>(FileIdentity), in_path.native().c_str(), FileIdentityLength);
  //  }
  //  void set_relative_file_name(_In_ CONST _Field_z_ WCHAR in_path[]) {
  //    RelativeFileName = new wchar_t[wcslen(in_path) + 1];
  //    wcscpy_s(const_cast<wchar_t*>(RelativeFileName), wcslen(in_path), in_path);
  //  }
};

class cloud_fetch_placeholders : public std::enable_shared_from_this<cloud_fetch_placeholders> {
 public:
  explicit cloud_fetch_placeholders(boost::asio::io_context&, FSys::path in_server_path, FSys::path in_child_path, CF_CALLBACK_INFO in_callback_info_, const CF_CALLBACK_PARAMETERS*)
      : server_path_{std::move(in_server_path)},
        search_path_{reinterpret_cast<wchar_t const*>(in_callback_info_.FileIdentity)},
        child_path_{std::move(in_child_path)},
        callback_info_{in_callback_info_} {}
  ~cloud_fetch_placeholders() = default;
  void async_run() {
    init();
    transfer_data(callback_info_.ConnectionKey, callback_info_.TransferKey);
  }

 private:
  struct data_value {
    std::wstring relative_file_name;
    std::wstring file_identity;
  };
  FSys::path server_path_;
  FSys::path search_path_;
  FSys::path child_path_;
  CF_CALLBACK_INFO callback_info_;
  std::vector<cf_placeholder_create_info> placeholder_create_infos_;
  std::vector<std::shared_ptr<data_value>> data_values_;

  NTSTATUS ntstatus_{STATUS_SUCCESS};

  void init() {
    server_path_.make_preferred();
    search_path_.make_preferred();
    child_path_.make_preferred();

    WIN32_FIND_DATA l_find_Data;
    HANDLE l_hfile_handle;

    auto l_search_path       = search_path_.native() + L"\\*";
    FSys::path l_parent_path = (child_path_ / search_path_.lexically_relative(server_path_)).make_preferred();
    if (l_parent_path.native().back() != L'\\') {
      l_parent_path += L"\\";
    }
    l_hfile_handle =
        ::FindFirstFileExW(l_search_path.c_str(), FindExInfoBasic, &l_find_Data, FindExSearchNameMatch, nullptr, 0);
    if (l_hfile_handle == INVALID_HANDLE_VALUE) {
      ntstatus_ = NTSTATUS_FROM_WIN32(::GetLastError());
      return;
    }

    do {
      if (l_find_Data.cFileName[0] == L'.' && (l_find_Data.cFileName[1] == L'\0' || l_find_Data.cFileName[1] == L'.')) {
        continue;
      }
      cf_placeholder_create_info& l_cloud_entry = placeholder_create_infos_.emplace_back();
      auto& l_data_value                        = data_values_.emplace_back(std::make_shared<data_value>());
      l_data_value->relative_file_name          = l_find_Data.cFileName;
      l_data_value->file_identity               = (l_parent_path / l_find_Data.cFileName).native();

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

    } while (::FindNextFileW(l_hfile_handle, &l_find_Data));

    ::FindClose(l_hfile_handle);
  }

  void transfer_data(_In_ CF_CONNECTION_KEY connectionKey, _In_ LARGE_INTEGER transferKey) {
    CF_OPERATION_INFO opInfo            = {0};
    CF_OPERATION_PARAMETERS opParams    = {0};

    opInfo.StructSize                   = sizeof(opInfo);
    opInfo.Type                         = CF_OPERATION_TYPE_TRANSFER_DATA;
    opInfo.ConnectionKey                = connectionKey;
    opInfo.TransferKey                  = transferKey;

    opParams.ParamSize                  = RTL_SIZEOF_THROUGH_FIELD(CF_OPERATION_PARAMETERS, TransferPlaceholders);

    opParams.TransferPlaceholders.Flags = CF_OPERATION_TRANSFER_PLACEHOLDERS_FLAG_NONE;
    opParams.TransferPlaceholders.CompletionStatus = ntstatus_;
    opParams.TransferPlaceholders.PlaceholderTotalCount =
        cloud_provider_registrar::longlong_to_large_integer(placeholder_create_infos_.size());
    opParams.TransferPlaceholders.PlaceholderArray = placeholder_create_infos_.data();
    opParams.TransferPlaceholders.PlaceholderCount = placeholder_create_infos_.size();
    opParams.TransferPlaceholders.EntriesProcessed = placeholder_create_infos_.size();

    THROW_IF_FAILED(::CfExecute(&opInfo, &opParams));
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

  try {
    auto l_ptr = std::make_shared<cloud_fetch_data>(
        g_io_context(), l_server_path, l_child_path, *callbackInfo, callbackParameters
    );
    l_cloud_provider_registrar->cloud_fetch_data_list.emplace(callbackInfo->FileId.QuadPart, l_ptr);
    l_ptr->async_read();
  } catch (const boost::system::system_error& in_error) {
    DOODLE_LOG_INFO(boost::diagnostic_information(in_error));
  }
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
  if (auto l_ptr = l_cloud_provider_registrar->cloud_fetch_data_list[callbackInfo->FileId.QuadPart]; !l_ptr.expired()) {
    l_ptr.lock()->cancel();
  }
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
  auto* l_cloud_provider_registrar = reinterpret_cast<cloud_provider_registrar*>(callbackInfo->CallbackContext);
  //  DOODLE_LOG_INFO("收到占位符请求 {} {}", callbackInfo->FileIdentity, callbackInfo->NormalizedPath);
  auto l_ptr                       = std::make_shared<cloud_fetch_placeholders>(
      g_io_context(), l_cloud_provider_registrar->server_path(), l_cloud_provider_registrar->child_path(),
      *callbackInfo, callbackParameters
  );
  l_ptr->async_run();
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
    THROW_IF_FAILED(
        ::CfCreatePlaceholders(l_parent_path.native().c_str(), &l_cloud_entry, 1, CF_CREATE_FLAG_STOP_ON_ERROR, nullptr)
    );
  } while (::FindNextFileW(l_hfile_handle, &l_find_Data));

  ::FindClose(l_hfile_handle);
}
void cloud_provider_registrar::create_placeholder(const FSys::path& in_parent) {
  auto l_path = in_parent;
  l_path.make_preferred();
  list_dir_info(l_path);
}

void cloud_provider_registrar::uninit2() {
  THROW_IF_FAILED(CfDisconnectSyncRoot(s_transferCallbackConnectionKey));
  THROW_IF_FAILED(CfUnregisterSyncRoot(root_.generic_wstring().c_str()));
}
}  // namespace doodle