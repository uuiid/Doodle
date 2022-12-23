//
// Created by TD on 2022/9/29.
//
#include "doodle_core/core/app_base.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/json_rpc/core/server.h>
#include <doodle_core/platform/win/get_prot.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_app/app/this_rpc_exe.h>

#include "doodle_lib/distributed_computing/client.h"
#include <doodle_lib/app/doodle_main_app.h>
#include <doodle_lib/app/rpc_server_facet.h>
#include <doodle_lib/distributed_computing/server.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/asio/post.hpp>
#include <boost/process.hpp>
#include <boost/test/unit_test.hpp>

#include <iostream>
#include <main_fixtures/lib_fixtures.h>

using namespace doodle;
struct loop_rpc {};
BOOST_FIXTURE_TEST_SUITE(rpc, loop_rpc)
BOOST_AUTO_TEST_CASE(base) {
  doodle_lib l_ib{};

  distributed_computing::server l_s{};
  l_s.run();
  distributed_computing::client l_c{};

  auto l_timer = std::make_shared<boost::asio::high_resolution_timer>(g_io_context());
  l_timer->expires_after(10s);
  l_timer->async_wait([l_timer, this](auto) {
    std::cout << "stop run"
              << "\n";
    g_io_context().stop();
  });
  l_c.call("test");
  g_io_context().run();
}

BOOST_AUTO_TEST_SUITE_END()
