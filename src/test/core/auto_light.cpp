//
// Created by TD on 2023/11/23.
//

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
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
using namespace doodle;
BOOST_AUTO_TEST_SUITE(auto_light_test)

BOOST_AUTO_TEST_CASE(utf16_test) {
  doodle::import_and_render_ue_ns::fix_project("D:\\test_files\\doodle_plug.uproject");
}

BOOST_AUTO_TEST_SUITE_END()