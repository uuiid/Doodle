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

  doodle::time_point_wrap my_t{};
  doodle::time_point_wrap my_t2{};
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

    REQUIRE(my_t.get_local_time() == doodle::chrono::clock_cast<doodle::chrono::local_t>(k_new));
  }

  SECTION("set local") {
    my_t.set_local_time(k_local);
    REQUIRE(my_t.get_utc_time() == k_new);
  }
  SECTION("time duration") {
    auto k_sys_time1 = chrono::local_days(2021_y / 7 / 21_d) + 10h + 45min + 30s;
    auto k_sys_time2 = chrono::local_days(2021_y / 7 / 23_d) + 16h + 20min + 30s;
    my_t.set_local_time(k_sys_time1);
    my_t2.set_local_time(k_sys_time2);
    using namespace Catch::literals;
    //    auto k_matcher = Catch::Approx(5.1);

    REQUIRE(my_t.work_duration(my_t2).count() == (20.583_a).epsilon(0.01));
    SECTION("time works durtion") {
      k_sys_time1 = chrono::local_days(2021_y / 7 / 21_d) + 10h + 45min + 30s;
      k_sys_time2 = chrono::local_days(2021_y / 7 / 27_d) + 16h + 20min + 30s;
      my_t.set_local_time(k_sys_time1);
      my_t2.set_local_time(k_sys_time2);
      REQUIRE(my_t.work_duration(my_t2).count() == (36.583_a).epsilon(0.01));
    }
    SECTION("ond day time"){
      k_sys_time1 = chrono::local_days(2021_y / 6 / 23_d) + 17h + 8min + 48s;
      k_sys_time2 = chrono::local_days(2021_y / 6 / 23_d) + 20h + 8min + 48s;
      my_t.set_local_time(k_sys_time1);
      my_t2.set_local_time(k_sys_time2);
      REQUIRE(my_t.work_duration(my_t2).count() == (0.86_a).epsilon(0.01));
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
#include <algorithm>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
TEST_CASE("test create metadata", "[server][metadata]") {
  using namespace doodle;
  std::string user{
      "宦倩冰 邰小溪 孔佳晨 顾柏文 乌书竹 邱媛女 冷梓颖"
      " 乔平良 闻爱萍 堵乐然 钭如之 屠宵月 农曦秀 苍佳洁 索优悦 黎晶晶"
      " 杜珠玉 苏若美 宦修美 弓博敏 巴沛蓝 邵河灵 靳佩兰 张子丹 鱼绮丽"
      " 胡怀亦 幸艳芳 衡韶丽 薛友绿 耿丝琪 杜俊慧 双芮悦 欧妙菡 陆梅雪"
      " 寿江红 益小凝 燕洮洮 古逸雅 宁梦华 扶倩愉 国宇丞 魏梅雪 冯诗蕊"
      " 刘秋双 宰怡丞 须奇文 蓟言文 邹心诺 陈曦秀 谢绣文 充靖柔 红凡梦"
      " 冷霞绮 郏怡若 庄书萱 谭布侬 罗芷蕾 籍子琳 吕向莉 贺颜落 蔚元瑶"
      " 冷绮云 家密思 钱云琼 养茵茵 鄂曼吟 璩静恬 步幸瑶 仰陶宁 秦芸静"
      " 温玉兰 潘华楚 金初瑶 孔蔚然 朱诗晗 相夏槐 秦小之 焦念薇 陆丽英"
      " 程香洁 万陶然 浦迎荷 隆智敏 潘蔓蔓 曾子芸 乌滢滢 古笑雯 顾半烟"
      " 宫津童 彭灵波 翟诗桃 须思聪 方碧玉 梁羡丽 漕闲华 韩皎月 扈晴丽"
      " 温燕平 冀冬梅 赖代容"};
  std::istringstream iss{user};
  string_list user_list{std::istream_iterator<std::string>(iss),
                        std::istream_iterator<std::string>()};
  std::random_device rd;
  std::mt19937 mt(rd());
  std::uniform_int_distribution<int> dist{1, 30};

  auto k_server = RpcServerHandle{};
  auto& set     = core_set::getSet();
  k_server.runServer(set.get_meta_rpc_port(), set.get_file_rpc_port());

  doodle_lib::Get().init_gui();
  auto k_fa = std::make_shared<metadata_factory>();

  std::vector<MetadataPtr> k_delete_id;
  SECTION("create project") {
    auto k_prj = std::make_shared<project>("D:/tmp", "case_tset");
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
        k_eps = std::make_shared<episodes>(k_prj, k_i);
        k_prj->child_item.push_back_sig(k_eps);
        k_eps->updata_db(k_fa);

        k_delete_id.push_back(k_eps);
        if (k_i % 2 == 0) {
          /// 生成镜头
          for (int k_j = 0; k_j < 10; ++k_j) {
            k_shot_ptr = std::make_shared<shot>(k_eps, k_j);
            k_eps->child_item.push_back_sig(k_shot_ptr);
            k_shot_ptr->updata_db(k_fa);

            k_delete_id.push_back(k_shot_ptr);

            if (k_j % 3 == 0) {
              /// 生成人名
              for (int k_k = 0; k_k < 10; ++k_k) {
                k_assets_ptr = std::make_shared<assets>(k_shot_ptr, fmt::format("tset_{}", k_k));
                k_shot_ptr->child_item.push_back_sig(k_assets_ptr);
                k_assets_ptr->updata_db(k_fa);

                k_delete_id.push_back(k_assets_ptr);
                if (k_k % 3 == 0) {
                  ///  生成具体条目
                  for (int k_l = 0; k_l < 20; ++k_l) {
                    auto k_file = std::make_shared<assets_file>(k_assets_ptr, k_assets_ptr->show_str());
                    k_assets_ptr->child_item.push_back_sig(k_file);

                    using namespace chrono::literals;
                    auto k_time = std::make_shared<time_point_wrap>(chrono::system_clock::now() - 3h * i);
                    DOODLE_LOG_INFO("生成时间 {} ", k_time->show_str());
                    ++i;

                    auto k_u_i = dist(mt);
                    k_file->set_time(k_time);
                    k_file->set_user(user_list[k_u_i]);
                    k_file->set_department(magic_enum::enum_cast<department>(k_u_i % 8).value());
                    /// 插入数据
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

TEST_CASE("gui action metadata", "[metadata][gui]") {
//  using namespace doodle;
//  using namespace doodle::chrono::literals;
//  auto k_server = RpcServerHandle{};
//  auto& set     = CoreSet::getSet();
//  k_server.runServer(set.getMetaRpcPort(), set.getFileRpcPort());
//
//  DoodleLib::Get().init_gui();
//  auto k_fa = std::make_shared<MetadataFactory>();
//
//  SECTION("export excel") {
//    auto k_ex = std::make_shared<actn_export_excel>();
//
//    k_ex->sig_get_arg.connect([]() {
//      actn_export_excel::arg k_arg{};
//      auto k_time_b = std::make_shared<time_point_wrap>();
//      k_time_b->set_local_time(chrono::local_days(2021_y / 6 / 1_d));
//      auto k_time_end = std::make_shared<time_point_wrap>();
//      k_time_end->set_local_time(chrono::local_days(2021_y / 6 / 30_d));
//      k_arg.p_time_range = std::make_pair(k_time_b, k_time_end);
//
//      k_arg.date = FSys::temp_directory_path() / "doodle_tset";
//      if (FSys::exists(k_arg.date))
//        FSys::create_directories(k_arg.date);
//
//      return k_arg;
//    });
//
//    (*k_ex)({}, {});
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
