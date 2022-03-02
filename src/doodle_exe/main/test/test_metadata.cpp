//
// Created by TD on 2021/7/27.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/client/client.h>
#include <doodle_lib/long_task/database_task.h>
#include <doodle_lib/app/app.h>

#include <catch.hpp>

using namespace doodle;

class test_convert : public app {
 public:
  void run_test() {
    auto reg   = g_reg();

    auto k_prj = make_handle(reg->create());
    auto& k_p  = k_prj.emplace<project>();
    reg->set<database::ref_data>(k_prj.emplace<database>());
    REQUIRE(k_prj.all_of<project, database>());

    auto& k_d = k_prj.get<database>();

    auto k_s  = make_handle(reg->create());
    auto& s   = k_s.emplace<shot>();
    k_s.emplace<database>();

    s.set_shot(1);
    s.set_shot_ab(shot::shot_ab_enum::A);

    REQUIRE(k_s.all_of<shot, database>());

    auto& k_d2 = k_s.get<database>();
    metadata_database k_data2;
    k_data2 = k_d2;
    std::cout << k_data2.user_data << std::endl;
    auto k_tmp2 = make_handle(reg->create());
    auto& k_d3  = k_tmp2.get_or_emplace<database>();
    k_d3        = k_data2;

    std::cout << "k_d3 id: " << k_d3.uuid() << std::endl;
    std::cout << "k_d2 id: " << k_d2.uuid() << std::endl;
    REQUIRE(k_d3 == k_d2);
  };
};

TEST_CASE_METHOD(test_convert, "convert") {
  this->run_test();
}

TEST_CASE("create_prj") {
  auto k_prj = make_handle();
  k_prj.emplace<project>("D:/tmp", "case_tset");
  core::client{}.new_project(k_prj);
}

TEST_CASE("open project") {
  core::client{}.open_project("D:/tmp");
  while (!g_main_loop().empty())
    g_main_loop().update({}, nullptr);
}

TEST_CASE("install project") {
  auto k_h = make_handle();
  k_h.emplace<process_message>();
  std::vector<entt::handle> k_l{};
  auto k_prj_h = make_handle();
  auto& k_prj  = k_prj_h.emplace<project>("D:/tmp", "test1");
  g_reg()->set<project>(k_prj);
  k_l.push_back(k_prj_h);
  g_main_loop().attach<database_task_install>(k_l);

  while (!g_main_loop().empty())
    g_main_loop().update({}, nullptr);
}
TEST_CASE("load project data") {
  g_main_loop().attach<database_task_select>("D:/tmp");
  while (!g_main_loop().empty())
    g_main_loop().update({}, nullptr);

  for (auto&& [e, p] : g_reg()->view<project>().each()) {
    std::cout << p.get_name() << std::endl;
  }
}

class name_data : public app_command_base {
 public:
  name_data() {
    std::random_device rd;
    mt.discard(rd());
  }
  std::uniform_int_distribution<int> dist{1, 30};
  std::mt19937 mt;

  void crete_data() {
    auto& set  = core_set::getSet();

    auto k_prj = make_handle(g_reg()->view<project>()[0]);

    for (size_t i = 0; i < 20; ++i) {
      if (i > 15) {
        for (size_t k = 0; k < 20; ++k) {
          auto k_ass = make_handle();
          k_ass.emplace<assets>(fmt::format("ues{}/a{}/test{}", i, k, i));
          k_ass.emplace<database>();
        }
      } else {
        entt::handle k_i1 = make_handle();
        k_i1.emplace<season>(std::int32_t(i % 5));
        k_i1.emplace<database>();
        k_i1.emplace<episodes>(i);

        for (size_t k = 0; k < 30; ++k) {
          entt::handle k_i2 = make_handle();
          k_i2.emplace<season>(std::int32_t(i % 5));
          k_i2.emplace<episodes>().set_episodes(k);
          auto& k_sho = k_i2.emplace<shot>();
          k_sho.set_shot(k);
          if (i % 2 == 0) {
            k_sho.set_shot_ab(shot::shot_ab_enum::B);
          }
          k_i2.emplace<assets_file>(fmt::format("test_{}_{}", i, k));
          k_i2.emplace<database>();
          k_i2.get_or_emplace<time_point_wrap>(chrono::system_clock::now() - 3h * i);
          auto k_u_i = dist(mt);
          k_i2.get<assets_file>().set_user(fmt::format("user_{}", k_u_i));
          k_i2.get<assets_file>().set_department(magic_enum::enum_cast<department>(k_u_i % 8).value());
        }
      }
    }
  }

  void create_project_and_open() {
    auto k_prj = make_handle();
    k_prj.emplace<project>("D:/tmp", "case_tset");
    k_prj.emplace<database>();
    g_reg()->set<project>(k_prj.get<project>());
    auto k_msg = make_handle();
    k_msg.emplace<process_message>();
    core::client{}.new_project(k_prj);
    g_main_loop()
        .attach<database_task_install>(k_prj)
        .then<one_process_t>([=]() {
          auto k_p = k_prj.get<project>().p_path;
          core::client{}.open_project(k_p);
        });
    while (!g_main_loop().empty())
      doodle::app_command_base::loop_one();
  }
};

TEST_CASE_METHOD(name_data, "create project data") {
  auto k_h = make_handle();
  k_h.emplace<project>();
  crete_data();
}
TEST_CASE_METHOD(name_data, "install project data") {
  create_project_and_open();

  crete_data();
  auto k_h = make_handle();
  k_h.emplace<process_message>();
  std::vector<entt::handle> k_l{};
  auto k_view = g_reg()->view<database>();
  std::transform(k_view.begin(), k_view.end(), std::back_inserter(k_l),
                 [](auto& in) {
                   return make_handle(in);
                 });
  g_main_loop().attach<database_task_install>(k_l);
  while (!g_main_loop().empty())
    g_main_loop().update({}, nullptr);
}

class test_time_duration : public app {
 public:
  chrono::sys_seconds p_new     = chrono::sys_days{2021_y / 06 / 16} + 10h + 34min + 37s;
  chrono::local_seconds p_local = chrono::local_days{2021_y / 06 / 16} + 18h + 34min + 37s;

  time_point_wrap time_1_a{chrono::local_days(2021_y / 7 / 21_d) + 10h + 45min + 30s};
  time_point_wrap time_1_b{chrono::local_days(2021_y / 7 / 23_d) + 16h + 20min + 30s};

  time_point_wrap time_2_a{chrono::local_days(2021_y / 7 / 21_d) + 10h + 45min + 30s};
  time_point_wrap time_2_b{chrono::local_days(2021_y / 7 / 27_d) + 16h + 20min + 30s};

  time_point_wrap time_3_a{chrono::local_days(2021_y / 6 / 23_d) + 17h + 8min + 48s};
  time_point_wrap time_3_b{chrono::local_days(2021_y / 6 / 23_d) + 20h + 8min + 48s};

  time_point_wrap time_4_a{chrono::sys_days(2021_y / 7 / 21_d) + 2h + 45min + 30s};
  time_point_wrap time_4_b{chrono::sys_days(2021_y / 7 / 23_d) + 8h + 20min + 30s};

  time_point_wrap time_5_a{chrono::sys_days(2021_y / 7 / 21_d) + 2h + 45min + 30s};
  time_point_wrap time_5_b{chrono::sys_days(2021_y / 7 / 27_d) + 8h + 20min + 30s};

  time_point_wrap time_6_a{chrono::sys_days(2021_y / 6 / 23_d) + 9h + 8min + 48s};
  time_point_wrap time_6_b{chrono::sys_days(2021_y / 6 / 23_d) + 12h + 8min + 48s};

  //  time_point_wrap time_7_a;
  //  time_point_wrap time_7_b;
  //
  //  time_point_wrap time_8_a;
  //  time_point_wrap time_8_b;
};

TEST_CASE_METHOD(test_time_duration, "test_time_duration1") {
  auto k_time = chrono::make_zoned(chrono::current_zone(), p_local);
  REQUIRE(k_time.get_sys_time() == p_new);
}

TEST_CASE_METHOD(test_time_duration, "test_time_duration2") {
  auto k_time = chrono::make_zoned(chrono::current_zone(), p_new);
  REQUIRE(k_time.get_local_time() == p_local);
}
using namespace Catch::literals;
TEST_CASE_METHOD(test_time_duration, "work_time") {
  REQUIRE(time_1_a.work_duration(time_1_b).count() == (20.583_a).epsilon(0.01));
  REQUIRE(time_2_a.work_duration(time_2_b).count() == (36.583_a).epsilon(0.01));
  REQUIRE(time_3_a.work_duration(time_3_b).count() == (0.86_a).epsilon(0.01));
  REQUIRE(time_4_a.work_duration(time_4_b).count() == (20.583_a).epsilon(0.01));
  REQUIRE(time_5_a.work_duration(time_5_b).count() == (36.583_a).epsilon(0.01));
  REQUIRE(time_6_a.work_duration(time_6_b).count() == (0.86_a).epsilon(0.01));
}

// TEST(DSTD, map_netDir) {
//   NETRESOURCE resources{};
//   resources.dwType       = RESOURCETYPE_DISK;
//   resources.lpLocalName  = (LPWSTR)L"S:";
//   resources.lpProvider   = 0;
//   resources.lpRemoteName = (LPWSTR)LR"(\\192.168.10.250\public\CangFeng)";
//   DWORD r                = WNetAddConnection2(&resources, NULL, NULL,
//                                CONNECT_TEMPORARY | CONNECT_INTERACTIVE | CONNECT_COMMANDLINE | CONNECT_CRED_RESET);
//   if (r != NO_ERROR) {
//     std::cout << r << std::endl;
//   }
//   ASSERT_TRUE(r == NO_ERROR);
// }
//
// TEST(DSTD, gset_netDir_name) {
//   TCHAR szDeviceName[150];
//   DWORD dwResult, cchBuff = sizeof(szDeviceName);
//
//   dwResult = WNetGetConnection(L"V:", szDeviceName, &cchBuff);
//
//   ASSERT_TRUE(dwResult == NO_ERROR);
//
//   std::wcout << std::wstring{szDeviceName} << std::endl;
//   auto rules_n = SetVolumeLabel(L"V:\\", L"test");
//   if (rules_n == 0) {
//     auto err = GetLastError();
//     std::cout << err << std::endl;
//   }
//   // ASSERT_TRUE(rules_n != 0);
//
//   wchar_t VolumeName[80];
//   auto rules = GetVolumeInformation(L"V:\\", VolumeName, sizeof(VolumeName), NULL, NULL, NULL, NULL, 0);
//   ASSERT_TRUE(rules);
//   std::cout << VolumeName << std::endl;
// }
//
// TEST(DSTD, canclel_netDir) {
//   DWORD r = WNetCancelConnection2(L"S:", CONNECT_UPDATE_PROFILE, true);
//   if (r != NO_ERROR) {
//     std::cout << r << std::endl;
//   }
//   ASSERT_TRUE(r == NO_ERROR);
// }
