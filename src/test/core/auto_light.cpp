//
// Created by TD on 2023/11/23.
//

#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_client/work.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/sqlite/task_sqlite_server.h>
#include <doodle_lib/http_method/task_info.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
using namespace doodle;
BOOST_AUTO_TEST_SUITE(auto_light)
constexpr std::array<const char*, 20> l_uuid_list = {
    "66291e09-36ac-43b6-97f7-8ca421054c57",
    "f980a323-a297-45dc-b14a-2dd5a947d958",
    "789e7426-6061-4f82-8fad-cbdaa7185c81",
    "38e130ea-bf50-4c11-8657-d4bcbb592e40",
    "f5914907-1200-48d8-a018-5299ff6c6502",
    "ae22fcf1-d143-42dd-afdc-ec1dc55cfc6e",
    "f42f973c-84bc-488c-809a-6f2860274acd",
    "72e2af7f-2584-4cc7-b4fe-00c0293b7ed6",
    "7e5057db-b251-4fde-aef9-42cff4cb15fa",
    "85ad4e4d-d6ad-4a6f-8d8c-b09c33a90d03",
    "c90e90e3-330d-4ad0-b61d-371da84e6cf9",
    "87eaf161-c7e2-462f-9c1b-daff77de16a8",
    "d30fac5f-97d7-4549-930d-e73604328f33",
    "4868e9ca-c7b0-4b26-a442-9be7136541f7",
    "85f3a5a8-df0f-4abf-98bf-b2fbce64894e",
    "34136195-12c2-4e8c-84d8-fcb171cf775f",
    "12756d9b-dab1-40e4-a42f-8933bb2791dc",
    "a9a91e1d-25e5-4d67-85bf-211b04819b74",
    "779878fb-662d-4027-a926-af368c73a175",
    "027724e7-5493-4a1c-8c95-a669f7eb3677"

};

BOOST_AUTO_TEST_CASE(server_and_works) {
  app_base l_app_base{};
  l_app_base.use_multithread(true);
  core_set::get_set().set_root("D:/kitsu/images");
  g_ctx().emplace<sqlite_database>().load("D:/kitsu_test.db");
  g_ctx().emplace<http::kitsu_ctx_t>("", "", "D:/kitsu/images");
  // 初始化路由
  auto l_rout_ptr = http::create_kitsu_route("E:/source/kitsu/dist");
  http::reg_computing_time(*l_rout_ptr);
  http::reg_dingding_attendance(*l_rout_ptr);
  // 开始运行服务器
  http::run_http_listener(g_io_context(), l_rout_ptr, 50025);

  std::vector<std::shared_ptr<http::http_work>> l_works{20ull};
  for (int i = 0; i < 20; ++i) {
    auto l_ptr = l_works.emplace_back(std::make_shared<http::http_work>());
    std::string l_uuid{l_uuid_list[i]};
    l_ptr->run("ws://127.0.0.1:50025/api/doodle/computer", "http://127.0.0.1:50025", boost::lexical_cast<uuid>(l_uuid));
  }
  l_app_base.run();
}

BOOST_AUTO_TEST_SUITE_END()