#include "doodle_core/logger/logger.h"
#include <doodle_core/doodle_core.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/phoenix.hpp>
#include <boost/phoenix/bind/bind_member_variable.hpp>
#include <boost/phoenix/core/reference.hpp>
#include <boost/phoenix/core/value.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>

#include <cstddef>
#include <entt/entity/fwd.hpp>
#include <fmt/core.h>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>
#include <string>
#include <vector>

// #include <boost/lambda/bind.hpp>

using namespace doodle;
BOOST_AUTO_TEST_CASE(ranges_ints) {
  auto l_view = ranges::views::ints(0, 10);

  BOOST_TEST(l_view.size() == 10);
  DOODLE_LOG_INFO(l_view);
}

BOOST_AUTO_TEST_CASE(ranges_ints_phoenix) {
  std::vector<std::size_t> l_test{0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
  auto l_view = ranges::views::ints(0, 10);

  // boost::phoenix::placeholders::

  BOOST_TEST(l_view.size() == 10);
  DOODLE_LOG_INFO(l_view);
}