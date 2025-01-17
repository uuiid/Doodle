//
// Created by TD on 2022/9/20.
//

#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/metadata/rules.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/time_tool/work_clock.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <boost/system/detail/error_code.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include <chrono>
#include <fmt/core.h>

using namespace doodle;

namespace {

struct time_cpp_suite {
  doodle_lib lib{};
};
}  // namespace
BOOST_FIXTURE_TEST_SUITE(tset_time, time_cpp_suite)

BOOST_AUTO_TEST_CASE(time_warp_fmt_test) {
  time_point_wrap l_time{2022, 5, 7, 11, 46, 55};
  chrono::sys_time_pos l_time_pos{l_time.get_sys_time()};
  chrono::zoned_time l_zoned_time{chrono::current_zone(), l_time_pos};
  chrono::zoned_time l_zoned_time2{chrono::current_zone(), chrono::system_clock::now()};

  BOOST_TEST_MESSAGE(fmt::format("{}", l_time));
  BOOST_TEST_MESSAGE(fmt::format("{}", l_time_pos));
  BOOST_TEST_MESSAGE(fmt::format("{}", l_zoned_time.get_local_time()));
  BOOST_TEST_MESSAGE(fmt::format("{}", l_zoned_time2.get_local_time()));
  BOOST_TEST(fmt::format("{}", l_time) != "2022-05-07 11:46:55"s);
  BOOST_TEST(fmt::format("{}", l_time) != "2022-05-07 19:46:55"s);

  BOOST_TEST(fmt::format("{:%Y/%m/%d %H:%M:%S}", l_time) != "2022/05/07 11:46:55"s);
  BOOST_TEST(fmt::format("{:%Y/%m/%d %H:%M:%S}", l_time) != "2022/05/07 19:46:55"s);
  BOOST_TEST_MESSAGE(fmt::format("{}", chrono::system_clock::now()));
}

BOOST_AUTO_TEST_CASE(time_to_json) {
  using namespace std::literals;
  chrono::system_zoned_time l_time{
      chrono::current_zone(), std::chrono::system_clock::time_point{chrono::sys_days{2022y / 5 / 7} + 11h + 46min + 55s}
  };
  auto l_time_sys = l_time.get_sys_time();
  auto l_t =
      nlohmann::json::parse(R"({"time":"2022-05-07 11:46:55"})")["time"].get<chrono::system_zoned_time>().get_sys_time(
      );
  BOOST_TEST(l_t == l_time_sys);

  auto l_t1 =
      nlohmann::json::parse(R"({"time":"2022-05-07T11:46:55Z"})")["time"].get<chrono::system_zoned_time>().get_sys_time(
      );
  BOOST_TEST(l_t1 == l_time_sys);

  auto l_t2 = nlohmann::json::parse(R"({"time":"2022-05-07T19:46:55+08:00"})")["time"]
                  .get<chrono::system_zoned_time>()
                  .get_sys_time();
  BOOST_TEST(l_t2 == l_time_sys);
}

BOOST_AUTO_TEST_SUITE_END()
