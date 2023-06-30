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

#include <SearchAPI.h>  // needed for AddFolderToSearchIndexer
#include <memory>
#include <oleidl.h>
#include <propkey.h>      // needed for ApplyTransferStateToFile
#include <propvarutil.h>  // needed for ApplyTransferStateToFile
#include <thread>
#include <unknwn.h>
#include <winrt/base.h>
#include <winrt/windows.applicationmodel.datatransfer.dragdrop.h>

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
};

BOOST_AUTO_TEST_CASE(wenrt_base_) {
  winrt::init_apartment();
  search_managet l_s{};

  std::thread{[]() { winrt::init_apartment(winrt::apartment_type::single_threaded); }}.detach();
}