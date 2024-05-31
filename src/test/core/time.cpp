//
// Created by TD on 2022/9/20.
//

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/core/doodle_lib.h"
#include "doodle_core/metadata/rules.h"
#include "doodle_core/metadata/user.h"
#include "doodle_core/time_tool/work_clock.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/pin_yin/convert.h>

#include "doodle_lib/attendance/attendance_rule.h"

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
  BOOST_TEST_MESSAGE(fmt::format("{}", l_time));
  BOOST_TEST_MESSAGE(fmt::format("{}", l_time_pos));
  BOOST_TEST_MESSAGE(fmt::format("{}", l_zoned_time.get_local_time()));
  BOOST_TEST(fmt::format("{}", l_time) != "2022-05-07 11:46:55"s);
  BOOST_TEST(fmt::format("{}", l_time) != "2022-05-07 19:46:55"s);

  BOOST_TEST(fmt::format("{:%Y/%m/%d %H:%M:%S}", l_time) != "2022/05/07 11:46:55"s);
  BOOST_TEST(fmt::format("{:%Y/%m/%d %H:%M:%S}", l_time) != "2022/05/07 19:46:55"s);
  BOOST_TEST_MESSAGE(fmt::format("{}", chrono::system_clock::now()));
}

BOOST_AUTO_TEST_CASE(time_warp_local_to_sys) {
  using namespace chrono::literals;
  time_point_wrap l_sys_time{time_point_wrap::time_point{chrono::sys_days{2022y / 7 / 30d} + chrono::hours{7}}};
  time_point_wrap l_local_time{
      time_point_wrap::time_local_point{chrono::local_days{2022y / 7 / 30d} + chrono::hours{7}}};

  auto l_time_du = chrono::duration_cast<chrono::hours>(l_sys_time - l_local_time);

  BOOST_TEST(l_time_du.count() == 8);
}
BOOST_AUTO_TEST_CASE(time_work) {
  business::work_clock work_clock{};

  business::attendance_rule l_r{};
  auto l_user = entt::handle{*g_reg(), g_reg()->create()};
  l_user.emplace<user>().set_name("test1");
  auto&& l_rules = l_user.emplace<business::rules>(business::rules::get_default());
  l_rules.extra_p.emplace_back(
      time_point_wrap{2022, 12, 18, 9}, time_point_wrap{2022, 12, 18, 10}, "18号9-10点调休一小时"s, true
  );
  l_rules.extra_p.emplace_back(
      time_point_wrap{2022, 12, 26, 9}, time_point_wrap{2022, 12, 26, 13}, "年假上午半天"s, false
  );
  l_rules.extra_p.emplace_back(
      time_point_wrap{2022, 12, 02, 9}, time_point_wrap{2022, 12, 02, 13}, "事假上午半天"s, false
  );
  l_rules.extra_p.emplace_back(
      time_point_wrap{2022, 12, 14, 9}, time_point_wrap{2022, 12, 14, 13}, "事假上午半天"s, false
  );
  l_rules.extra_p.emplace_back(
      time_point_wrap{2022, 12, 23, 9}, time_point_wrap{2022, 12, 23, 13}, "事假上午半天"s, false
  );

  l_rules.extra_p.emplace_back(
      time_point_wrap{2022, 12, 18, 9}, time_point_wrap{2022, 12, 18, 18}, "18号加班一天"s, true
  );
  time_point_wrap l_time{2022, 12, 1};

  l_r.async_get_work_clock(
      l_user, l_time.current_month_start(), l_time.current_month_end(),
      [l_user](const boost::system::error_code& in_code, const business::work_clock& in_clock) {
        BOOST_TEST_MESSAGE(fmt::format("用户 {} 时间规则 {}", l_user.get<user>().get_name(), in_clock.debug_print()));
      }
  );
}

BOOST_AUTO_TEST_SUITE_END()
