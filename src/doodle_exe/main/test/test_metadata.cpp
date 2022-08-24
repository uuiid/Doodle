//
// Created by TD on 2021/7/27.
//
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/app/app.h>
#include <doodle_core/core/core_sig.h>
#include <doodle_core/time_tool/work_clock.h>

#include <catch.hpp>

using namespace doodle;

class test_convert : public app {
 public:
  void run_test() {
    auto reg   = g_reg();

    auto k_prj = make_handle();
    auto& k_p  = k_prj.emplace<project>();
    reg->ctx().emplace<database::ref_data>(k_prj.emplace<database>());
    REQUIRE(k_prj.all_of<project, database>());

    auto& k_d = k_prj.get<database>();

    auto k_s  = make_handle(reg->create());
    auto& s   = k_s.emplace<shot>();
    k_s.emplace<database>();

    s.set_shot(1);
    s.set_shot_ab(shot::shot_ab_enum::A);

    REQUIRE(k_s.all_of<shot, database>());

    auto& k_d2  = k_s.get<database>();

    auto k_tmp2 = make_handle(reg->create());
    auto& k_d3  = k_tmp2.get_or_emplace<database>();

    std::cout << "k_d3 id: " << k_d3.uuid() << std::endl;
    std::cout << "k_d2 id: " << k_d2.uuid() << std::endl;
    REQUIRE(k_d3 == k_d2);
  };
  void make_josn() {
    auto l_h = make_handle();
    l_h.emplace<database>();
    l_h.emplace<assets_file>();
    l_h.emplace<episodes>();
    l_h.emplace<shot>();
    l_h.emplace<season>();
    l_h.emplace<assets>("test/ass");

    auto l_h_ref = make_handle();

    l_h.emplace<ref_meta>().ref_list.emplace_back(l_h_ref.emplace<database>());

    nlohmann::json l_json{};
    entt_tool::save_comm<
        assets_file,
        episodes,
        shot,
        season,
        assets,
        ref_meta>(l_h, l_json);
    FSys::ofstream{"out_json.json"}
        << l_json.dump();
  }
};

TEST_CASE_METHOD(test_convert, "convert") {
  this->run_test();
}

TEST_CASE_METHOD(test_convert, "test_out_json") {
  this->make_josn();
}

TEST_CASE("create_prj") {
  auto k_prj = make_handle();
  k_prj.emplace<project>("D:/tmp", "case_tset");
  core::client{}.new_project(k_prj);
}

TEST_CASE("open project") {
  //  core::client{}.open_project("D:/tmp");
  //  while (!g_main_loop().empty())
  //    g_main_loop().update({}, nullptr);
}

TEST_CASE("install project") {
  auto k_h = make_handle();
  k_h.emplace<process_message>();
  std::vector<entt::handle> k_l{};
  auto k_prj_h = make_handle();
  auto& k_prj  = k_prj_h.emplace<project>("D:/tmp", "test1");
  g_reg()->ctx().emplace<project>(k_prj);
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
          k_i2.replace<time_point_wrap>(chrono::system_clock::now() - 3h * i);
          auto k_u_i = dist(mt);
          k_i2.get<assets_file>().set_user(fmt::format("user_{}", k_u_i));
          k_i2.get<assets_file>().organization_p = fmt::to_string(k_u_i);
        }
      }
    }
  }

  void create_project_and_open() {
    auto k_prj = make_handle();
    k_prj.emplace<project>("D:/tmp", "case_tset");
    k_prj.emplace<database>();
    g_reg()->ctx().emplace<project>(k_prj.get<project>());
    auto k_msg = make_handle();
    k_msg.emplace<process_message>();
    core::client{}.new_project(k_prj);
    g_main_loop()
        .attach<database_task_install>(k_prj)
        .then<one_process_t>([=]() {
          auto k_p = k_prj.get<project>().p_path;
          //          core::client{}.open_project(k_p);
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

class test_o_snapshot {
 public:
  test_o_snapshot() = default;
  void operator()(const entt::entity& in_e){
      //    std::cout << fmt::format("get entt::entity: {}", in_e) << std::endl;
  };
  void operator()(const std::underlying_type_t<entt::entity>& in_v) {
    std::cout << fmt::format("get std::underlying_type_t<entt::entity>: {}", in_v) << std::endl;
  };
  template <class T>
  void operator()(const entt::entity& in_entity, const T& in_t) {
    nlohmann::json l_j{};
    l_j["entt"]   = in_entity;
    l_j["t_data"] = in_t;
    std::cout << l_j.dump() << std::endl;
  };
};

class test_metadata_install : public app {
 public:
  void make_install() {
    test_o_snapshot l_out{};
    //    std::cout << fmt::format("entt::tombstone_t: {}", (entt::entity)entt::tombstone) << std::endl;
    entt::snapshot{*g_reg()}
        .entities(l_out)
        .component<project, season, episodes, shot, assets, assets_file, time_point_wrap>(l_out);
  };
};

TEST_CASE_METHOD(test_metadata_install, "test_metadata_install") {
  g_reg()->ctx().at<core_sig>().project_end_open.connect([this](auto...) { this->make_install(); });

  this->run();
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
