//
// Created by TD on 2023/11/23.
//

#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/file_association.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/down_auto_light_anim_file.h>
#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/up_auto_light_file.h>
#include <doodle_lib/doodle_lib_all.h>
#include <doodle_lib/exe_warp/import_and_render_ue.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue_exe.h>
#include <doodle_lib/http_client/work.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/http_snapshot.h>
#include <doodle_lib/http_method/task_info.h>
#include <doodle_lib/http_method/task_server.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

using namespace doodle;

BOOST_AUTO_TEST_CASE(auto_light) {
  app_base l_app_base{};
  g_pool_db().set_path("D:/test_files/test_db/test.db");
  {
    auto l_db_conn = g_pool_db().get_connection();
    g_ctx().emplace<http::task_server>().init(l_db_conn);
  }

  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::computer::reg(*l_rout_ptr);
  http::task_info::reg(*l_rout_ptr);
  // 开始运行服务器
  auto l_listener = std::make_shared<http::http_listener>(g_io_context(), l_rout_ptr);
  l_listener->run();
  g_ctx().get<http::task_server>().run();
  try {
    g_io_context().run();

  } catch (const std::exception& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
}