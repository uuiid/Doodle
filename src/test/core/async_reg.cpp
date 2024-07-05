#include <doodle_core/core/app_base.h>
#include <doodle_core/core/async_reg.h>
#include <doodle_core/doodle_core_fwd.h>

#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

using namespace doodle;

BOOST_AUTO_TEST_SUITE(coroutine)

BOOST_AUTO_TEST_CASE(async_reg) {
  app_base l_app_base{};

  l_app_base.run();
}
BOOST_AUTO_TEST_SUITE_END()