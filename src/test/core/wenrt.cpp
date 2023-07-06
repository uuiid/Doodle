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
#include <doodle_cloud_drive/cloud/cloud_provider_registrar.h>
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