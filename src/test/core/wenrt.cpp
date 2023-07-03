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
  cloud_provider_registrar() { init2(); }
  ~cloud_provider_registrar() { uninit2(); }

  void connect_sync_root_transfer_callbacks();
  static void CALLBACK
  on_fetch_data(_In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters);
  static void CALLBACK on_cancel_fetch_data(
      _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
  );

 private:
  CF_CONNECTION_KEY s_transferCallbackConnectionKey{};

  void init() {
    auto l_sync_root_id = get_sync_root_id();

    winrt::Windows::Storage::Provider::StorageProviderSyncRootInfo l_info{};
    l_info.Id(l_sync_root_id);
    auto l_folder = winrt::Windows::Storage::StorageFolder::GetFolderFromPathAsync(child_path).get();
    l_info.Path(l_folder);
    l_info.DisplayNameResource(L"sy");

    l_info.IconResource(L"%SystemRoot%\\system32\\charmap.exe,0");
    l_info.HydrationPolicy(winrt::Windows::Storage::Provider::StorageProviderHydrationPolicy::Full);
    l_info.HydrationPolicyModifier(winrt::Windows::Storage::Provider::StorageProviderHydrationPolicyModifier::None);
    l_info.PopulationPolicy(winrt::Windows::Storage::Provider::StorageProviderPopulationPolicy::AlwaysFull);
    l_info.InSyncPolicy(
        winrt::Windows::Storage::Provider::StorageProviderInSyncPolicy::FileCreationTime |
        winrt::Windows::Storage::Provider::StorageProviderInSyncPolicy::DirectoryCreationTime
    );
    l_info.Version(L"0.0.0");
    l_info.ShowSiblingsAsGroup(false);
    l_info.HardlinkPolicy(winrt::Windows::Storage::Provider::StorageProviderHardlinkPolicy::None);

    winrt::Windows::Foundation::Uri uri(L"http://cloudmirror.example.com/recyclebin");
    l_info.RecycleBinUri(uri);

    std::wstring syncRootIdentity(server_path);
    syncRootIdentity.append(L"->");
    syncRootIdentity.append(child_path);

    winrt::Windows::Storage::Streams::IBuffer contextBuffer =
        winrt::Windows::Security::Cryptography::CryptographicBuffer::ConvertStringToBinary(
            syncRootIdentity.data(), winrt::Windows::Security::Cryptography::BinaryStringEncoding::Utf8
        );
    l_info.Context(contextBuffer);

    winrt::Windows::Foundation::Collections::IVector<
        winrt::Windows::Storage::Provider::StorageProviderItemPropertyDefinition>
        customStates = l_info.StorageProviderItemPropertyDefinitions();

    winrt::Windows::Storage::Provider::StorageProviderSyncRootManager::Register(l_info);
  }

  void init2() {
    // 使用win32 api注册同步文件夹
    //    auto l_sync_root_id         = get_sync_root_id();
    // a3dde735-edc1-404d-87a2-a506db7b9c36
    //    GUID const l_guid           = {0xA3DDE735, 0xEDC1, 0x404D, {0x87, 0xA2, 0xA5, 0x6B, 0xB7, 0xB9, 0xC3, 0x6D}};
    GUID l_guid                 = {0};
    l_guid.Data1                = 0xA3DDE735;
    l_guid.Data2                = 0xEDC1;
    l_guid.Data3                = 0x404D;
    //    l_guid.Data4                = {0x87, 0xA2, 0xA5, 0x6B, 0xB7, 0xB9, 0xC3, 0x6D};
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
    HRESULT hr =
        CfRegisterSyncRoot(child_path, &l_reg, &policies, CF_REGISTER_FLAG_DISABLE_ON_DEMAND_POPULATION_ON_ROOT);
    try {
      THROW_IF_FAILED(hr);
    }
    CATCH_LOG()
  }

  void uninit2() {
    THROW_IF_FAILED(CfDisconnectSyncRoot(s_transferCallbackConnectionKey));
    HRESULT hr = CfUnregisterSyncRoot(child_path);
    THROW_IF_FAILED(hr);
  }

  void uninit() {
    auto l_sync_root_id = get_sync_root_id();
    winrt::Windows::Storage::Provider::StorageProviderSyncRootManager::Unregister(l_sync_root_id);
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

void cloud_provider_registrar::connect_sync_root_transfer_callbacks() {
  static CF_CALLBACK_REGISTRATION s_MirrorCallbackTable[] = {
      {CF_CALLBACK_TYPE_FETCH_DATA, cloud_provider_registrar::on_fetch_data},
      {CF_CALLBACK_TYPE_CANCEL_FETCH_DATA, cloud_provider_registrar::on_cancel_fetch_data},
      CF_CALLBACK_REGISTRATION_END};

  THROW_IF_FAILED(::CfConnectSyncRoot(
      child_path, s_MirrorCallbackTable, nullptr,
      CF_CONNECT_FLAG_REQUIRE_PROCESS_INFO | CF_CONNECT_FLAG_REQUIRE_FULL_FILE_PATH, &s_transferCallbackConnectionKey
  ));
};

void CALLBACK cloud_provider_registrar::on_fetch_data(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}
void CALLBACK cloud_provider_registrar::on_cancel_fetch_data(
    _In_ CONST CF_CALLBACK_INFO* callbackInfo, _In_ CONST CF_CALLBACK_PARAMETERS* callbackParameters
) {}

BOOST_AUTO_TEST_CASE(wenrt_base_) {
  //  winrt::init_apartment();
  //  search_managet l_s{};
  cloud_provider_registrar l_reg{};

  l_reg.connect_sync_root_transfer_callbacks();
  BOOST_TEST(true);
  //  std::thread{[]() { winrt::init_apartment(winrt::apartment_type::single_threaded); }}.detach();
}