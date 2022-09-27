//
// Created by TD on 2022/9/20.
//

#define BOOST_TEST_MODULE doodle lib

#include <boost/test/included/unit_test.hpp>
#include <doodle_core/pin_yin/convert.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/time_point_wrap.h>

using namespace doodle;

struct loop_fixtures {
  loop_fixtures()  = default;
  ~loop_fixtures() = default;

  doodle_lib l_lib{};
  void setup() {
    doodle_lib::create_time_database();
    BOOST_TEST_MESSAGE("完成夹具设置");
  };
  void teardown(){};
};

BOOST_FIXTURE_TEST_SUITE(fmt_print, loop_fixtures)

BOOST_AUTO_TEST_CASE(time_warp_fmt_test) {
  time_point_wrap l_time{2022, 5, 7, 3, 46, 55};
  BOOST_TEST_MESSAGE(fmt::format("{}", l_time));
  BOOST_TEST(fmt::format("{}", l_time) == "2022-05-07 11:46:55"s);
  BOOST_TEST(fmt::format("{:S}", l_time) == "2022-05-07 03:46:55"s);
  BOOST_TEST(fmt::format("{:L}", l_time) == "2022-05-07 11:46:55"s);

  BOOST_TEST(
      fmt::format("{:%Y/%m/%d %H:%M:%S}", l_time) ==
      "2022/05/07 11:46:55"s
  );
  BOOST_TEST(
      fmt::format("{:S%Y/%m/%d %H:%M:%S}", l_time) ==
      "2022/05/07 03:46:55"s
  );
  BOOST_TEST(
      fmt::format("{:L%Y/%m/%d %H:%M:%S}", l_time) ==
      "2022/05/07 11:46:55"s
  );
}

BOOST_AUTO_TEST_CASE(time_warp_local_to_sys) {
  using namespace chrono::literals;
  time_point_wrap l_sys_time{
      time_point_wrap::time_point{
          chrono::sys_days{2022_y / 7 / 30_d} + chrono::hours{7}}};
  time_point_wrap l_local_time{
      time_point_wrap::time_local_point{
          chrono::local_days{2022_y / 7 / 30_d} + chrono::hours{7}}};

  auto l_time_du = chrono::duration_cast<chrono::hours>(l_sys_time - l_local_time);

  BOOST_TEST(l_time_du.count() == 8);
}
BOOST_AUTO_TEST_SUITE_END()
