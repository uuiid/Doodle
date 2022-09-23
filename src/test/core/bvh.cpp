//
// Created by TD on 2022/9/23.
//
#define BOOST_TEST_MODULE doodle lib

#include <boost/test/included/unit_test.hpp>
#include <doodle_core/pin_yin/convert.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/metadata/time_point_wrap.h>

#include <boost/spirit.hpp>
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

BOOST_AUTO_TEST_CASE(test_base_spirit) {
  std::vector<std::double_t> l_num;
  auto l_p = boost::spirit::real_p[boost::spirit::push_back_a(l_num)] >> *(',' >> boost::spirit::real_p[boost::spirit::push_back_a(l_num)]);
  boost::spirit::parse("1,2,3,4,5", l_p, boost::spirit::space_p);
  BOOST_TEST((l_num == std::vector<std::double_t>{1, 2, 3, 4, 5}));
}

BOOST_AUTO_TEST_SUITE_END()
