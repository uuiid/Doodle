//
// Created by TD on 2022/8/26.
//

#define BOOST_TEST_MODULE doodle pinying
#include <boost/test/unit_test.hpp>
#include <doodle_core/pin_yin/convert.h>

using namespace doodle;
BOOST_AUTO_TEST_CASE(free_test_function)
/* Compare with void free_test_function() */
{
  BOOST_TEST(convert::Get().toEn("测试"s) == "ceshi");
  BOOST_TEST(convert::Get().toEn("林奇"s) == "linqi");
  BOOST_TEST(convert::Get().toEn("dsa??__测试"s) == "dsa??__ceshi");
}
