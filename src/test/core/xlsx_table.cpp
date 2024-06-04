#include <doodle_core/core/doodle_lib.h>

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/computing_time.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

BOOST_AUTO_TEST_SUITE(xlsx_table)

BOOST_AUTO_TEST_CASE(computing_time) {
  app_base l_app_base{};
  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::reg_computing_time(*l_rout_ptr);
  // 开始运行服务器
  auto l_listener = std::make_shared<http::http_listener>(g_thread().executor(), l_rout_ptr, 50023);
  l_listener->run();
  try {
    // 工作守卫
    auto l_work_guard = boost::asio::make_work_guard(g_io_context());
    g_io_context().run();
  } catch (const std::exception& e) {
    BOOST_TEST_MESSAGE(e.what());
  }
}

BOOST_AUTO_TEST_SUITE_END()