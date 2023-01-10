//
// Created by TD on 2022/10/26.
//
#include <doodle_core/platform/win/get_prot.h>
#include <doodle_core/time_tool/work_clock.h>

#include <doodle_lib/core/holidaycn_time.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <main_fixtures/lib_fixtures.h>

using namespace doodle;

BOOST_FIXTURE_TEST_CASE(test_holidaycn, lib_fixtures) {
  business::work_clock work_clock{};
  holidaycn_time l_time{};
  l_time.set_clock(work_clock);

  DOODLE_LOG_INFO(work_clock.debug_print());

  auto l_time_du = work_clock(time_point_wrap{2022, 10, 1}, time_point_wrap{2022, 10, 3});

  BOOST_TEST((l_time_du == chrono::seconds{0}));
}

BOOST_AUTO_TEST_CASE(test_port) {
  BOOST_TEST(win::has_tcp_port(80));
  BOOST_TEST(!win::has_tcp_port(23333));
}
