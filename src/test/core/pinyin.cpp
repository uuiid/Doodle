//
// Created by TD on 2022/8/26.
//

#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>

using namespace doodle;
BOOST_AUTO_TEST_CASE(test_pingyi) {
  BOOST_TEST(convert::Get().toEn("测试"s) == "ceshi");
  BOOST_TEST(convert::Get().toEn("林奇"s) == "linqi");
  BOOST_TEST(convert::Get().toEn("dsa??__测试"s) == "dsa??__ceshi");
}
