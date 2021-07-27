//
// Created by TD on 2021/7/27.
//
#include <DoodleLib/DoodleLib.h>

#include <catch.hpp>

TEST_CASE("time duration", "[metadata]") {
  using namespace doodle;
  using namespace doodle::chrono::literals;
  auto k_new   = chrono::sys_days{2021_y / 06 / 16} + 10h + 34min + 37s;
  auto k_local = chrono::local_days{2021_y / 06 / 16} + 18h + 34min + 37s;
  //  REQUIRE(doodle::TimeDuration{}.getUTCTime() == chrono::system_clock::now());
  SECTION("date time sys") {
    auto k_time = chrono::make_zoned(chrono::current_zone(), k_new);
    REQUIRE(k_time.get_local_time() == k_local);
  }
  SECTION("date time local") {
    auto k_time = chrono::make_zoned(chrono::current_zone(), k_local);
    REQUIRE(k_time.get_sys_time() == k_new);
  }

  doodle::TimeDuration my_t{};
  doodle::TimeDuration my_t2{};
  SECTION("time set") {
    my_t.set_year(2021);
    my_t.set_month(6);
    my_t.set_day(16);
    my_t.set_hour(10);
    my_t.set_minutes(34);
    my_t.set_second(37);
    REQUIRE(my_t.get_year() == 2021);
    REQUIRE(my_t.get_month() == 6);
    REQUIRE(my_t.get_day() == 16);
    REQUIRE(my_t.get_hour() == 10);
    REQUIRE(my_t.get_minutes() == 34);
    REQUIRE(my_t.get_second() == 37);

    REQUIRE(my_t.getLocalTime() == k_new);
  }

  SECTION("set local") {
    my_t.set_local_time(k_local);
    REQUIRE(my_t.getUTCTime() == k_new);
  }
  SECTION("time duration") {
    auto k_sys_time1 = chrono::local_days(2020_y / 7 / 21_d) + 10h + 45min + 30s;
    auto k_sys_time2 = chrono::local_days(2020_y / 7 / 23_d) + 16h + 20min + 30s;
    my_t.set_local_time(k_sys_time1);
    my_t2.set_local_time(k_sys_time2);
    using namespace Catch::literals;
    //    auto k_matcher = Catch::Approx(5.1);

    REQUIRE(my_t.work_duration(my_t2).count() == (21.583_a).epsilon(0.01));
    SECTION("time works durtion") {
      k_sys_time1 = chrono::local_days(2020_y / 7 / 21_d) + 10h + 45min + 30s;
      k_sys_time2 = chrono::local_days(2020_y / 7 / 27_d) + 16h + 20min + 30s;
      my_t.set_local_time(k_sys_time1);
      my_t2.set_local_time(k_sys_time2);
      REQUIRE(my_t.work_duration(my_t2).count() == (37.583_a).epsilon(0.01));
    }
  }
}

TEST_CASE("observable_container", "[metadata][observable]") {
  using namespace doodle;
  //  using my_str = observable_container<std::vector<std::string>, details::pre<std::vector<std::string> > >;
  //  details::pre<std::vector<std::string> > k_pre{};
  //  my_str test{&k_pre};

  using my_str = observable_container<std::vector<std::string> >;
  my_str test{};

  test.sig_clear.connect([]() { std::cout << "clear" << std::endl; });
  test.sig_insert.connect([](const std::string& val) { std::cout << val << std::endl; });
  test.sig_erase.connect([](const std::string& val) { std::cout << val << std::endl; });
  test.sig_push_back.connect([](const std::string& val) { std::cout << val << std::endl; });
  test.sig_resize.connect([](std::size_t where) { std::cout << where << std::endl; });
  test.sig_swap.connect([](const std::vector<std::string>& strs) {
    for (auto& s : strs) {
      std::cout << s << ",";
    }
    std::cout << std::endl;
  });

  test.push_back_sig({"test"});
  test.push_back_sig({"tesat"});
  test.push_back_sig({"22222"});
  test.insert_sig(test.begin(), {"11111"});
  test.erase_sig(test.begin());
  test.resize_sig(2);
  auto k_other = std::vector<std::string>{"1", "2", "3", "4"};
  test.swap_sig(k_other);
  test.clear_sig();
}

TEST_CASE("test create metadata", "[server][metadata]") {
  using namespace doodle;
  auto k_server = RpcServerHandle{};
  auto& set     = CoreSet::getSet();
  k_server.runServer(set.getMetaRpcPort(), set.getFileRpcPort());

  set.guiInit();
  auto k_fa = std::make_shared<MetadataFactory>();

  std::vector<MetadataPtr> k_delete_id;
  SECTION("create project") {
    auto k_prj = std::make_shared<Project>("D:/tmp", "case_tset");
    k_prj->insert_into(k_fa);
    REQUIRE(k_prj->getId() != 0);
    DOODLE_LOG_INFO("prj id is {} ", k_prj->getId());

    k_delete_id.push_back(k_prj);

    SECTION("create other") {
      EpisodesPtr k_eps{};
      ShotPtr k_shot_ptr{};
      AssetsPtr k_assets_ptr{};
      auto i = 1;
      /// 生成集数
      for (int k_i = 0; k_i < 10; ++k_i) {
        k_eps = std::make_shared<Episodes>(k_prj, k_i);
        k_prj->child_item.push_back_sig(k_eps);
        k_eps->updata_db(k_fa);

        k_delete_id.push_back(k_eps);
        if (k_i % 2 == 0) {
          /// 生成镜头
          for (int k_j = 0; k_j < 10; ++k_j) {
            k_shot_ptr = std::make_shared<Shot>(k_eps, k_j);
            k_eps->child_item.push_back_sig(k_shot_ptr);
            k_shot_ptr->updata_db(k_fa);

            k_delete_id.push_back(k_shot_ptr);

            if (k_j % 3 == 0) {
              /// 生成人名
              for (int k_k = 0; k_k < 10; ++k_k) {
                k_assets_ptr = std::make_shared<Assets>(k_shot_ptr, fmt::format("tset_{}", k_k));
                k_shot_ptr->child_item.push_back_sig(k_assets_ptr);
                k_assets_ptr->updata_db(k_fa);

                k_delete_id.push_back(k_assets_ptr);
                if (k_k % 3 == 0) {
                  ///  生成具体条目
                  for (int k_l = 0; k_l < 20; ++k_l) {
                    auto k_d    = chrono::days{30} / (20 * 3 * 3 * 5);
                    auto k_file = std::make_shared<AssetsFile>(k_assets_ptr, k_assets_ptr->showStr());
                    auto k_time = std::make_shared<TimeDuration>(chrono::system_clock::now() + k_d * i);
                    ++i;
                    k_file->setTime(k_time);
                    k_file->updata_db(k_fa);
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  //  for (auto& k_m : k_delete_id) {
  //    k_m->deleteData(k_fa);
  //  }
}

//TEST(DSTD, map_netDir) {
//  NETRESOURCE resources{};
//  resources.dwType       = RESOURCETYPE_DISK;
//  resources.lpLocalName  = (LPWSTR)L"S:";
//  resources.lpProvider   = 0;
//  resources.lpRemoteName = (LPWSTR)LR"(\\192.168.10.250\public\CangFeng)";
//  DWORD r                = WNetAddConnection2(&resources, NULL, NULL,
//                               CONNECT_TEMPORARY | CONNECT_INTERACTIVE | CONNECT_COMMANDLINE | CONNECT_CRED_RESET);
//  if (r != NO_ERROR) {
//    std::cout << r << std::endl;
//  }
//  ASSERT_TRUE(r == NO_ERROR);
//}
//
//TEST(DSTD, gset_netDir_name) {
//  TCHAR szDeviceName[150];
//  DWORD dwResult, cchBuff = sizeof(szDeviceName);
//
//  dwResult = WNetGetConnection(L"V:", szDeviceName, &cchBuff);
//
//  ASSERT_TRUE(dwResult == NO_ERROR);
//
//  std::wcout << std::wstring{szDeviceName} << std::endl;
//  auto rules_n = SetVolumeLabel(L"V:\\", L"test");
//  if (rules_n == 0) {
//    auto err = GetLastError();
//    std::cout << err << std::endl;
//  }
//  // ASSERT_TRUE(rules_n != 0);
//
//  wchar_t VolumeName[80];
//  auto rules = GetVolumeInformation(L"V:\\", VolumeName, sizeof(VolumeName), NULL, NULL, NULL, NULL, 0);
//  ASSERT_TRUE(rules);
//  std::cout << VolumeName << std::endl;
//}
//
//TEST(DSTD, canclel_netDir) {
//  DWORD r = WNetCancelConnection2(L"S:", CONNECT_UPDATE_PROFILE, true);
//  if (r != NO_ERROR) {
//    std::cout << r << std::endl;
//  }
//  ASSERT_TRUE(r == NO_ERROR);
//}
