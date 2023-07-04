//
// Created by TD on 2022/8/26.
//

#include "doodle_core/logger/logger.h"
#include "doodle_core/metadata/assets_file.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include "fmt/core.h"
// clang-format off


#include <memory>
#include <sddl.h>
#include <string>
#include <thread>
#include <unknwn.h>
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

class cloud_provider_registrar {
 public:
  cloud_provider_registrar(const FSys::path& in_root, const FSys::path& in_server_path)
      : root_{in_root}, server_root_{in_server_path} {
    init2();
  }
  ~cloud_provider_registrar() { uninit2(); }

  void connect_sync_root_transfer_callbacks();
  static void CALLBACK
  on_fetch_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);
  static void CALLBACK on_cancel_fetch_data(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );

 private:
  CF_CONNECTION_KEY s_transferCallbackConnectionKey{};
  FSys::path root_{};
  FSys::path server_root_{};

  std::vector<CF_PLACEHOLDER_CREATE_INFO> list_dir_info(const FSys::path& in_parent);
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
    policies.HardLink           = CF_HARDLINK_POLICY_ALLOWED;
    policies.Hydration.Primary  = CF_HYDRATION_POLICY_PARTIAL;
    policies.InSync             = CF_INSYNC_POLICY_NONE;
    policies.Population.Primary = CF_POPULATION_POLICY_PARTIAL;
    THROW_IF_FAILED(CfRegisterSyncRoot(
        root_.generic_wstring().c_str(), &l_reg, &policies, CF_REGISTER_FLAG_DISABLE_ON_DEMAND_POPULATION_ON_ROOT
    ));
    static CF_CALLBACK_REGISTRATION s_MirrorCallbackTable[] = {
        {CF_CALLBACK_TYPE_FETCH_DATA, cloud_provider_registrar::on_fetch_data},
        {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cloud_provider_registrar::on_cancel_fetch_data},
        CF_CALLBACK_REGISTRATION_END};

    THROW_IF_FAILED(::CfConnectSyncRoot(
        child_path, s_MirrorCallbackTable, nullptr,
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

void CALLBACK cloud_provider_registrar::on_fetch_data(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void CALLBACK cloud_provider_registrar::on_cancel_fetch_data(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
std::vector<CF_PLACEHOLDER_CREATE_INFO> cloud_provider_registrar::list_dir_info(const FSys::path& in_parent) {
  std::vector<CF_PLACEHOLDER_CREATE_INFO> l_r{};

  WIN32_FIND_DATA L_find_Data;
  HANDLE L_hfile_handle;
  CF_PLACEHOLDER_CREATE_INFO cloudEntry;
}

BOOST_AUTO_TEST_CASE(wenrt_base_) {
  //  winrt::init_apartment();
  //  search_managet l_s{};
  cloud_provider_registrar l_reg{child_path, server_path};

  BOOST_TEST(true);
  //  std::thread{[]() { winrt::init_apartment(winrt::apartment_type::single_threaded); }}.detach();
}