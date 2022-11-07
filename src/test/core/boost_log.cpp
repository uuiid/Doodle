// #include <boost/log/core.hpp>
#define BOOST_USE_WINAPI_VERSION BOOST_WINAPI_VERSION_WIN7
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include <main_fixtures/lib_fixtures.h>

BOOST_AUTO_TEST_CASE(test_boost_log) {
  // BOOST_WINAPI_VERSION_WIN6;
  BOOST_LOG_TRIVIAL(trace) << "A trace severity message";
  BOOST_LOG_TRIVIAL(debug) << "A debug severity message";
  BOOST_LOG_TRIVIAL(info) << "An informational severity message";
  BOOST_LOG_TRIVIAL(warning) << "A warning severity message";
  BOOST_LOG_TRIVIAL(error) << "An error severity message";
  BOOST_LOG_TRIVIAL(fatal) << "A fatal severity message";
}