//
// Created by TD on 24-7-15.
//
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/http_client_core.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

using namespace doodle;
BOOST_AUTO_TEST_SUITE(http_test)

BOOST_AUTO_TEST_CASE(quequ_t) {
  app_base l_app_base{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};
  auto l_c = std::make_shared<http::detail::http_client_data_base>(g_io_context());
  l_c->init("https://baidu.com/",&l_ctx);
  boost::beast::http::request<boost::beast::http::empty_body> l_req{boost::beast::http::verb::get, "/", 11};
  l_req.prepare_payload();
  auto l_f1 = boost::asio::co_spawn(
      g_io_context(), http::detail::read_and_write<boost::beast::http::string_body>(l_c, l_req), boost::asio::use_future
  );
  auto l_f2 =    boost::asio::co_spawn(
      g_io_context(), http::detail::read_and_write<boost::beast::http::string_body>(l_c, l_req), boost::asio::use_future
  );
  g_io_context().run();

  auto [l_ec1, l_r1] = l_f1.get();
  BOOST_TEST(!l_ec1);
  auto [l_ec2, l_r2] = l_f2.get();
  BOOST_TEST(!l_ec2);
}

BOOST_AUTO_TEST_SUITE_END()