#include <doodle_core/core/doodle_lib.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/sqlite/kitsu_backend_sqlite.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

BOOST_AUTO_TEST_SUITE(xlsx_table)

constexpr static std::string_view g_token{
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJmcmVzaCI6ZmFsc2UsImlhdCI6MTcxNzU1MDUxMywianRpIjoiOTU0MDg1NjctMzE1OS00Y2MzLTljM2ItZmNiMzQ4MTIwNjU5IiwidHlwZSI6Im"
    "FjY2VzcyIsInN1YiI6ImU5OWMyNjZhLTk1ZjUtNDJmNS1hYmUxLWI0MTlkMjk4MmFiMCIsIm5iZiI6MTcxNzU1MDUxMywiZXhwIjoxNzY0NjMzNjAw"
    "LCJpZGVudGl0eV90eXBlIjoiYm90In0.xLV17bMK8VH0qavV4Ttbi43RhaBqpc1LtTUbRwu1684"
};

BOOST_AUTO_TEST_CASE(computing_time) {
  app_base l_app_base{};

  auto& l_save = g_ctx().emplace<http::kitsu_backend_sqlite>();

  {
    g_pool_db().set_path("D:/test_files/test_db/test2.db");
    auto l_db_conn = g_pool_db().get_connection();
    l_save.init(l_db_conn);
  }
  auto l_client = g_ctx().emplace<std::shared_ptr<kitsu::kitsu_client>>(
      std::make_shared<kitsu::kitsu_client>("192.168.40.182", "80")
  );
  l_client->set_access_token(std::string{g_token});
  

  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::reg_computing_time(*l_rout_ptr);
  http::reg_dingding_attendance(*l_rout_ptr);

  // 开始运行服务器
  auto l_listener = std::make_shared<http::http_listener>(g_thread().executor(), l_rout_ptr, 50023);
  l_listener->run();
  l_save.run();
  try {
    // 工作守卫
    auto l_work_guard = boost::asio::make_work_guard(g_io_context());
    g_io_context().run();
  } catch (const std::exception& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
}

BOOST_AUTO_TEST_SUITE_END()