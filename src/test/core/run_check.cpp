//
// Created by TD on 24-9-6.
//
#include <doodle_app/app/app_command.h>

#include "doodle_lib/exe_warp/maya_exe.h"
#include "doodle_lib/exe_warp/ue_exe.h"
#include <doodle_lib/exe_warp/check_files.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
BOOST_AUTO_TEST_SUITE(run_check)
using namespace doodle;

BOOST_AUTO_TEST_CASE(base) {
  app_command l_app{};
  FSys::path l_check{boost::unit_test::framework::master_test_suite().argv[1]};
  // 将测试的服务器地址改为测试ip
  core_set::get_set().server_ip = "http://192.168.40.180:50026";
  core_set_init{}.config_to_user();
  core_set_init{}.read_file();
  g_ctx().emplace<maya_ctx>();
  g_ctx().emplace<ue_ctx>();
  boost::asio::co_spawn(
      g_io_context(), check_files(l_check, spdlog::default_logger()),
      [](std::exception_ptr e, std::tuple<boost::system::error_code, std::string> res) { app_base::Get().stop_app(); }
  );
  l_app.run();
}

BOOST_AUTO_TEST_SUITE_END()
