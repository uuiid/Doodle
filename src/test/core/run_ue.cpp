//
// Created by td_main on 2023/9/6.
//

#include "doodle_core/core/app_base.h"

#include "doodle_lib/exe_warp/ue_exe.h"
#include <doodle_lib/doodle_lib_fwd.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
#include <boost/asio.hpp>
using namespace doodle;

BOOST_AUTO_TEST_CASE(core_run_ue) {
  app_base l_base{};
  g_ctx().emplace<ue_ctx>();
  core_set_init{}.config_to_user();
  core_set_init{}.read_file();
  boost::asio::co_spawn(g_io_context(),
                        async_run_ue({"--version"}, spdlog::default_logger()), boost::asio::detached);
  g_io_context().run();
}