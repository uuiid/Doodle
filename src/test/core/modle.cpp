//
// Created by TD on 24-8-6.
//

#include <doodle_core/core/app_base.h>
#include <doodle_lib/http_method/model_base.h>
#include <doodle_lib/core/http/http_listener.h>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>
BOOST_AUTO_TEST_SUITE(modle)
BOOST_AUTO_TEST_CASE(model_base) {
  using namespace doodle;

  app_base l_app_base{};
  l_app_base.use_multithread();

  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::model_base_reg(*l_rout_ptr);
  http::run_http_listener(g_io_context(), l_rout_ptr, 50027);
  l_app_base.run();

}
BOOST_AUTO_TEST_SUITE_END()