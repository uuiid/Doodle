//
// Created by TD on 2022/9/29.
//
#include <doodle_core/doodle_core.h>
#include <doodle_core/json_rpc/core/server.h>
#include <doodle_core/platform/win/get_prot.h>

#include <doodle_app/app/app_command.h>
#include <doodle_app/app/facet/gui_facet.h>
#include <doodle_app/app/this_rpc_exe.h>

#include <doodle_lib/app/doodle_main_app.h>
#include <doodle_lib/app/rpc_server_facet.h>
#include <doodle_lib/long_task/image_to_move.h>

#include <boost/process.hpp>
#include <boost/test/unit_test.hpp>

#include <main_fixtures/lib_fixtures.h>
#include <doodle_lib/distributed_computing/server.h>

using namespace doodle;
struct loop_rpc {};
BOOST_FIXTURE_TEST_SUITE(rpc, loop_rpc)
BOOST_AUTO_TEST_CASE(base) {







}

BOOST_AUTO_TEST_SUITE_END()
