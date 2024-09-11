//
// Created by TD on 2023/11/23.
//

#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/http_method/sqlite/task_sqlite_server.h>
#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/work.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/task_info.h>
#include <doodle_lib/http_method/task_server.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

using namespace doodle;
BOOST_AUTO_TEST_SUITE(auto_light)
BOOST_AUTO_TEST_CASE(only_server) {
  app_base l_app_base{};
  g_pool_db().set_path("D:/test_files/test_db/test.db");
  {
    auto l_db_conn = g_pool_db().get_connection();
    g_ctx().emplace<http::task_sqlite_server>().init(l_db_conn);
    g_ctx().emplace<http::task_server>().init();
  }

  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::computer_reg(*l_rout_ptr);
  http::task_info_reg(*l_rout_ptr);
  // 开始运行服务器
  http::run_http_listener(g_io_context(), l_rout_ptr, 50023);
  g_ctx().get<http::task_server>().run();
  g_ctx().get<http::task_sqlite_server>().run();

  try {
    // 工作守卫
    auto l_work_guard = boost::asio::make_work_guard(g_io_context());
    g_io_context().run();
  } catch (const std::exception& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
}

BOOST_AUTO_TEST_CASE(only_works) {
  app_base l_app_base{};
  std::vector<std::shared_ptr<http::http_work>> l_works{};
  for (int i = 0; i < 15; ++i) {
    auto l_work = std::make_shared<http::http_work>();
    l_work->run("http://127.0.0.1:50023/v1/computer");
    l_works.emplace_back(std::move(l_work));
  }
  try {
    // 工作守卫
    auto l_work_guard = boost::asio::make_work_guard(g_io_context());
    g_io_context().run();
  } catch (const std::exception& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
}

BOOST_AUTO_TEST_CASE(server_and_works) {
  app_base l_app_base{};
  g_pool_db().set_path("D:/test_files/test_db/test.db");
  {
    auto l_db_conn = g_pool_db().get_connection();
    g_ctx().emplace<http::task_sqlite_server>().init(l_db_conn);
    g_ctx().emplace<http::task_server>().init();
  }

  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::computer_reg(*l_rout_ptr);
  http::task_info_reg(*l_rout_ptr);
  // 开始运行服务器
  http::run_http_listener(g_io_context(), l_rout_ptr, 50023);
  g_ctx().get<http::task_server>().run();
  g_ctx().get<http::task_sqlite_server>().run();

  std::vector<std::shared_ptr<http::http_work>> l_works{};
  for (int i = 0; i < 1; ++i) {
    auto l_work = std::make_shared<http::http_work>();
    l_work->run("http://127.0.0.1:50023/v1/computer");
    l_works.emplace_back(std::move(l_work));
  }

  try {
    // 工作守卫
    auto l_work_guard = boost::asio::make_work_guard(g_io_context());
    g_io_context().run();
  } catch (const std::exception& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
}

BOOST_AUTO_TEST_SUITE_END()