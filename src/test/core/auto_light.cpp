//
// Created by TD on 2023/11/23.
//

#include "doodle_core/core/core_set.h"
#include "doodle_core/core/global_function.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/lib_warp/boost_fmt_error.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/main_map.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/scan_win_service.h>
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
#include <doodle_lib/http_method/kitsu.h>

#include <boost/asio.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
using namespace doodle;
BOOST_AUTO_TEST_SUITE(auto_light_test)

BOOST_AUTO_TEST_CASE(utf16_test) {
  doodle::import_and_render_ue_ns::fix_project("D:\\test_files\\doodle_plug.uproject");
}

BOOST_AUTO_TEST_CASE(install_plug_test) {
  app_base l_app_base{};
  core_set::get_set().server_ip   = "http://192.168.40.181";
  core_set::get_set().ue4_path    = "D:/Program Files/Epic Games/UE_5.5";
  core_set::get_set().ue4_version = "5.5";
  auto l_path                     = FSys::path{"E:/ue4.27"};
  boost::asio::co_spawn(g_io_context(), doodle::ue_exe_ns::install_doodle_plug(l_path), [](std::exception_ptr e) {
    if (e) {
      try {
        std::rethrow_exception(e);
      } catch (const std::exception& ex) {
        BOOST_TEST_MESSAGE(ex.what());
        BOOST_TEST(false);
      }
    } else {
      BOOST_TEST(true);
    }
  });

  l_app_base.run();
}

BOOST_AUTO_TEST_SUITE_END()