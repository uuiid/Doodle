//
// Created by TD on 24-9-24.
//
#include <doodle_lib/core/scan_win_service.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

namespace doodle::scan {
std::ostream& boost_test_print_type(std::ostream& ostr, scan_key_t const& right) {
  return ostr << right.name_ << right.number_ << right.version_name_;
}

}  // namespace doodle::scan

BOOST_AUTO_TEST_SUITE(metadata)
using namespace doodle;
BOOST_AUTO_TEST_CASE(scan_key_t) {
  scan::scan_key_t l_k1{}, l_k2{};

  BOOST_TEST(l_k1 == l_k2);
}

BOOST_AUTO_TEST_SUITE_END()