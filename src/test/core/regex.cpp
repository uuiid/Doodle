//
// Created by TD on 2023/2/6.
//
//
// Created by TD on 2023/1/14.
//
#include <doodle_core/doodle_core_fwd.h>

#include <boost/test/unit_test.hpp>

#include <regex>
using namespace doodle;

BOOST_AUTO_TEST_CASE(regex_maya) {
  std::regex l_regex{R"(JG_(\d+)_(\d+)_AN_Final)"};
  std::string l_fmt{"EP$1_$2_"};
  std::string l_string{"JG_011_084_AN_Final.ma"};
  auto l_out = std::regex_replace(l_string, l_regex, l_fmt);
  BOOST_TEST(l_out == "EP011_084"s);
}