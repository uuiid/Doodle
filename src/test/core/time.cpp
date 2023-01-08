//
// Created by TD on 2022/9/20.
//

#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>

#include <chrono>
#include <date/date.h>
#include <date/tz.h>
#include <main_fixtures/lib_fixtures.h>

using namespace doodle;

struct loop_fixtures : lib_fixtures {
  void setup() { BOOST_TEST_MESSAGE("完成夹具设置"); };
  void teardown(){};
};

BOOST_FIXTURE_TEST_SUITE(tset_time, loop_fixtures)

BOOST_AUTO_TEST_CASE(date_) {
  using namespace date::literals;
  using namespace std::literals;
  /// utc时间为 2022/05/07 3:46:55
  auto l_time_l = time_point_wrap{date::local_days{2022_y / 5 / 7_d} + 11h + 46min + 55s};
  /// utc时间为 2022/05/07 11:46:55
  auto l_time_s = time_point_wrap{date::sys_days{2022_y / 5 / 7_d} + 11h + 46min + 55s};
  /// utc时间为 2022/05/07 11:46:55
  auto l_time_3 = chrono::time_point<chrono::system_clock, chrono::seconds>{
      chrono::round<chrono::seconds>(l_time_l.get_local_time().time_since_epoch())};
  BOOST_TEST(
      date::format("%Y/%m/%d %H:%M:%S", date::local_days{2022_y / 5 / 7_d} + 11h + 46min + 55s) ==
      "2022/05/07 11:46:55"s
  );
  BOOST_TEST(
      date::format("%Y/%m/%d %H:%M:%S", date::sys_days{2022_y / 5 / 7_d} + 11h + 46min + 55s) == "2022/05/07 11:46:55"s
  );

  BOOST_TEST(date::format("%Y/%m/%d %H:%M:%S", l_time_l.get_sys_time()) == "2022/05/07 03:46:55.0000000"s);
  BOOST_TEST(date::format("%Y/%m/%d %H:%M:%S", l_time_s.get_sys_time()) == "2022/05/07 11:46:55.0000000"s);
  BOOST_TEST(date::format("%Y/%m/%d %H:%M:%S", l_time_3) == "2022/05/07 11:46:55"s);

  BOOST_TEST(date::format("%Y/%m/%d %H:%M:%S", l_time_3 + 8h) == fmt::format("{:%Y/%m/%d %H:%M:%S}", l_time_3));
}

BOOST_AUTO_TEST_CASE(time_warp_fmt_test) {
  time_point_wrap l_time{2022, 5, 7, 11, 46, 55};
  BOOST_TEST_MESSAGE(fmt::format("{}", l_time));
  BOOST_TEST(fmt::format("{}", l_time) == "2022-05-07 11:46:55"s);
  BOOST_TEST(fmt::format("{:S}", l_time) == "2022-05-07 03:46:55"s);
  BOOST_TEST(fmt::format("{:L}", l_time) == "2022-05-07 11:46:55"s);

  BOOST_TEST(fmt::format("{:%Y/%m/%d %H:%M:%S}", l_time) == "2022/05/07 11:46:55"s);
  BOOST_TEST(fmt::format("{:S%Y/%m/%d %H:%M:%S}", l_time) == "2022/05/07 03:46:55"s);
  BOOST_TEST(fmt::format("{:L%Y/%m/%d %H:%M:%S}", l_time) == "2022/05/07 11:46:55"s);
}

BOOST_AUTO_TEST_CASE(time_warp_local_to_sys) {
  using namespace chrono::literals;
  time_point_wrap l_sys_time{time_point_wrap::time_point{chrono::sys_days{2022_y / 7 / 30_d} + chrono::hours{7}}};
  time_point_wrap l_local_time{
      time_point_wrap::time_local_point{chrono::local_days{2022_y / 7 / 30_d} + chrono::hours{7}}};

  auto l_time_du = chrono::duration_cast<chrono::hours>(l_sys_time - l_local_time);

  BOOST_TEST(l_time_du.count() == 8);
}
BOOST_AUTO_TEST_SUITE_END()
