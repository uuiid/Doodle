//
// Created by TD on 2022/8/26.
//

#include "doodle_core/core/global_function.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/assets_file.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/pin_yin/convert.h>

#include "boost/asio/buffer.hpp"
#include "boost/asio/execution/context.hpp"
#include "boost/asio/execution_context.hpp"
#include "boost/asio/signal_set.hpp"
#include "boost/asio/windows/object_handle.hpp"
#include "boost/asio/windows/overlapped_ptr.hpp"
#include "boost/numeric/conversion/cast.hpp"
#include "boost/winapi/file_management.hpp"
#include <boost/asio.hpp>
#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/system_timer.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include "fmt/core.h"
// clang-format off


#include <algorithm>
#include <atomic>
#include <cwchar>
#include <memory>
#include <sddl.h>
#include <string>
#include <thread>
#include <type_traits>
#include <unknwn.h>
#include <utility>
#include <vector>
#include <wil/result.h>


#include <windows.h>
#include <winternl.h>
#include <comutil.h>
#include <oleidl.h>
#include <ntstatus.h>
#include <cfapi.h>

#include <SearchAPI.h>  // needed for AddFolderToSearchIndexer
#include <propkey.h>      // needed for ApplyTransferStateToFile
#include <propvarutil.h>  // needed for ApplyTransferStateToFile
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/windows.security.cryptography.h>
#include <winrt/windows.storage.compression.h>
#include <winrt/windows.storage.provider.h>

// clang-format on

auto child_path  = L"D:/sy";
auto server_path = L"E:/Doodle";
using namespace doodle;
class search_managet {
 public:
  search_managet() {
    winrt::com_ptr<ISearchManager> l_search_manager;
    winrt::check_hresult(
        CoCreateInstance(__uuidof(CSearchManager), nullptr, CLSCTX_SERVER, IID_PPV_ARGS(l_search_manager.put()))
    );
    winrt::com_ptr<ISearchCatalogManager> l_catalog_manager;
    winrt::check_hresult(l_search_manager->GetCatalog(L"SystemIndex", l_catalog_manager.put()));
    winrt::com_ptr<ISearchCrawlScopeManager> l_crawl_manager;
    winrt::check_hresult(l_catalog_manager->GetCrawlScopeManager(l_crawl_manager.put()));

    winrt::check_hresult(
        l_crawl_manager->AddDefaultScopeRule(L"file:///D:/sy/", TRUE, FOLLOW_FLAGS::FF_INDEXCOMPLEXURLS)
    );
    winrt::check_hresult(l_crawl_manager->SaveAll());
  }
};

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
        BOOST_TEST_MESSAGE(ec.what());
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
    ;
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

class cloud_provider_registrar {
 public:
  cloud_provider_registrar(FSys::path in_root, const FSys::path& in_server_path)
      : root_{std::move(in_root)}, server_root_{in_server_path} {
    if (!FSys::exists(root_)) FSys::create_directories(root_);
    init2();
    create_placeholder(in_server_path);
  }
  ~cloud_provider_registrar() { uninit2(); }

  /**
   * @brief CF_CALLBACK_TYPE_FETCH_DATA 此回调用于向同步提供程序询问所需的一系列文件数据，以满足占位符上的 I/O
   * 请求或显式水合请求。 如果同步提供程序指定的水合策略在同步根注册时不是 ALWAYS_FULL，则需要实现此回调。
   * @param
   */
  static void CALLBACK
  on_fetch_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);
  /**
   * @brief CF_CALLBACK_TYPE_VALIDATE_DATA
   * 此回调用于请求同步提供程序确认给定范围的文件数据（之前的CF_OPERATION_TYPE_TRANSFER_DATA操作已存在于磁盘上）是否有效，
   * 因此平台可以使用它来满足用户 I/O
   * 请求。仅当同步提供程序在同步根注册时指定水合策略修饰符VALIDATION_REQUIRED时，才需要实现此回调。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK
  on_validate_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);
  /**
   * @brief CF_CALLBACK_TYPE_CANCEL_FETCH_DATA
   * 此回调用于通知同步提供程序不再需要一系列文件数据，通常是因为原始请求已被取消。
   * 这允许同步提供程序停止花费精力尝试获取数据（取消未完成的网络请求等）。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK on_cancel_fetch_data(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );
  /**
   * @brief CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS
   * 此回调用于要求同步提供程序提供有关占位符目录内容的信息，以满足目录查询操作或尝试打开目录下的文件。
   * 仅当同步提供程序在同步根注册时指定除CF_POPULATION_POLICY_ALWAYS_FULL之外的策略时，才需要实现此回调。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK on_fetch_placeholders(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );
  /**
   * @brief 此回调用于通知同步提供程序其同步根之一下的占位符已成功打开以进行读/写/删除访问。
   * 执行打开操作的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK on_notify_file_open_completion(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );
  /**
   * @brief CF_CALLBACK_TYPE_NOTIFY_FILE_CLOSE_COMPLETION
   * 此回调用于通知同步提供程序，先前为读/写/删除访问而打开的同步根之一下的占位符现在已关闭。
   * 执行关闭的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK on_notify_file_close_completion(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );
  /**
   * @brief CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE 此回调用于通知同步提供程序其同步根之一下的占位符即将脱水。
   * 执行脱水的用户应用程序被阻止。预计同步提供商会做出响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK
  on_notify_dehydrate(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);
  /**
   * @brief CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE_COMPLETION 此回调用于通知同步提供程序其同步根之一下的占位符已成功脱水。
   * 执行脱水的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK on_notify_dehydrate_completion(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );
  /**
   * @brief CF_CALLBACK_TYPE_NOTIFY_DELETE 此回调用于通知同步提供程序其同步根之一下的占位符即将被删除。
   * 执行删除的用户应用程序被阻止。预计同步提供商会做出响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK
  on_notify_delete(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);
  /**
   * @brief CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION
   * 此回调用于通知同步提供程序其同步根之一下的占位符已成功删除。执行删除的用户应用程序不会被阻止。
   * 预计同步提供商不会做出任何响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK on_notify_delete_completion(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );
  /**
   * CF_CALLBACK_TYPE_NOTIFY_RENAME 此回调用于通知同步提供程序其同步根之一下的占位符即将被重命名或移动。
   * 执行重命名/移动的用户应用程序被阻止。预计同步提供商会做出响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK
  on_notify_rename(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);
  /**
   * @brief CF_CALLBACK_TYPE_NOTIFY_RENAME_COMPLETION
   * 此回调用于通知同步提供程序其同步根之一下的占位符已成功重命名或移动。
   * 执行重命名/移动的用户应用程序不会被阻止。预计同步提供商不会做出任何响应。此回调的实现是可选的。
   * @param callbackInfo
   * @param callbackParameters
   */
  static void CALLBACK on_notify_rename_completion(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );

 private:
  CF_CONNECTION_KEY s_transferCallbackConnectionKey{};
  FSys::path root_{};
  FSys::path server_root_{};

  inline static LARGE_INTEGER file_time_to_large_integer(_In_ const FILETIME in_filetime) {
    LARGE_INTEGER l_large_integer{};

    l_large_integer.LowPart  = in_filetime.dwLowDateTime;
    l_large_integer.HighPart = in_filetime.dwHighDateTime;

    return l_large_integer;
  };

  void list_dir_info(const FSys::path& in_parent);

  void create_placeholder(const FSys::path& in_parent);
  void init2() {
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
        {CF_CALLBACK_TYPE_FETCH_DATA, cloud_provider_registrar::on_fetch_data},
        {CF_CALLBACK_TYPE_VALIDATE_DATA, cloud_provider_registrar::on_validate_data},
        {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cloud_provider_registrar::on_cancel_fetch_data},
        {CF_CALLBACK_TYPE_FETCH_PLACEHOLDERS, cloud_provider_registrar::on_fetch_placeholders},
        {CF_CALLBACK_TYPE_NOTIFY_FILE_OPEN_COMPLETION, cloud_provider_registrar::on_notify_file_open_completion},
        {CF_CALLBACK_TYPE_NOTIFY_FILE_CLOSE_COMPLETION, cloud_provider_registrar::on_notify_file_close_completion},
        {CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE, cloud_provider_registrar::on_notify_dehydrate},
        {CF_CALLBACK_TYPE_NOTIFY_DEHYDRATE_COMPLETION, cloud_provider_registrar::on_notify_dehydrate_completion},
        {CF_CALLBACK_TYPE_NOTIFY_DELETE_COMPLETION, cloud_provider_registrar::on_notify_delete_completion},
        {CF_CALLBACK_TYPE_NOTIFY_DELETE, cloud_provider_registrar::on_notify_delete},
        {CF_CALLBACK_TYPE_NOTIFY_RENAME, cloud_provider_registrar::on_notify_rename},
        {CF_CALLBACK_TYPE_NOTIFY_RENAME_COMPLETION, cloud_provider_registrar::on_notify_rename_completion},
        CF_CALLBACK_REGISTRATION_END};

    THROW_IF_FAILED(::CfConnectSyncRoot(
        child_path, s_MirrorCallbackTable, this,
        CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH, &s_transferCallbackConnectionKey
    ));
  }

  void uninit2() {
    THROW_IF_FAILED(CfDisconnectSyncRoot(s_transferCallbackConnectionKey));
    THROW_IF_FAILED(CfUnregisterSyncRoot(root_.generic_wstring().c_str()));
  }

  static winrt::com_array<wchar_t> convert_sid_to_string_sid(::PSID p_sid) {
    winrt::com_array<wchar_t> l_sid_string;
    if (!::ConvertSidToStringSidW(p_sid, winrt::put_abi(l_sid_string))) {
      throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
    }
    return l_sid_string;
  }

  std::wstring get_sync_root_id() {
    auto l_token_user{get_token_information()};
    auto l_sid_string = convert_sid_to_string_sid(l_token_user->User.Sid);
    return fmt::format(L"Doodle.Drive.!{}!", l_sid_string);
  }

  std::unique_ptr<::TOKEN_USER> get_token_information() {
    std::unique_ptr<::TOKEN_USER> l_token_info{};
    auto l_token_handle{::GetCurrentThreadEffectiveToken()};
    DWORD token_info_length{};
    if (!::GetTokenInformation(l_token_handle, ::TokenUser, nullptr, 0, &token_info_length)) {
      if (::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
      }

      l_token_info.reset(reinterpret_cast<::TOKEN_USER*>(new char[token_info_length]));
      if (!::GetTokenInformation(
              l_token_handle, ::TokenUser, l_token_info.get(), token_info_length, &token_info_length
          )) {
        throw std::system_error{static_cast<int>(::GetLastError()), std::system_category()};
      }
    }
    return l_token_info;
  }
};
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
void CALLBACK cloud_provider_registrar::on_fetch_data(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void CALLBACK cloud_provider_registrar::on_cancel_fetch_data(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_validate_data(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_fetch_placeholders(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_file_open_completion(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_file_close_completion(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_dehydrate(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_delete(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_delete_completion(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_rename(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_dehydrate_completion(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void cloud_provider_registrar::on_notify_rename_completion(
    const CF_CALLBACK_INFO* callbackInfo, const CF_CALLBACK_PARAMETERS* callbackParameters
) {}
BOOST_AUTO_TEST_CASE(cloud_provider_registrar_base_1) {
  //  winrt::init_apartment();
  //  search_managet l_s{};
  cloud_provider_registrar l_reg{child_path, server_path};

  BOOST_TEST(true);
  //  std::thread{[]() { winrt::init_apartment(winrt::apartment_type::single_threaded); }}.detach();
}

BOOST_AUTO_TEST_CASE(directory_watcher_base_1) {
  //  winrt::init_apartment();
  //  search_managet l_s{};
  boost::asio::io_context l_io_context{};
  directory_watcher l_reg{l_io_context, "D:\\job"};
  //  boost::asio::any_io_executor l_any = l_io_context.get_executor();
  boost::asio::signal_set l_set{l_io_context, SIGINT, SIGTERM};
  l_set.async_wait([&](const boost ::system ::error_code& error, int signal_number) { l_reg.cancel(); });
  l_reg.read_changes_async();
  l_io_context.run();
  //  boost::asio::windows::object_handle l_h{};
  BOOST_TEST(true);
  //  std::thread{[]() { winrt::init_apartment(winrt::apartment_type::single_threaded); }}.detach();
}