//
// Created by TD on 2022/8/26.
//


#include <boost/test/unit_test.hpp>
#include <doodle_core/pin_yin/convert.h>
#include <doodle_core/doodle_core.h>

using namespace doodle;
BOOST_AUTO_TEST_CASE(create_lib)
/* Compare with void free_test_function() */
{
  doodle_lib l_lib{};

  BOOST_TEST((l_lib == doodle_lib::Get()));
  //  BOOST_TEST(convert::Get().toEn("林奇"s) == "linqi");
  //  BOOST_TEST(convert::Get().toEn("dsa??__测试"s) == "dsa??__ceshi");
}
