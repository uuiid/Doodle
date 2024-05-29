#include <doodle_core/core/doodle_lib.h>

#include <doodle_lib/http_client/dingding_client.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;
BOOST_AUTO_TEST_SUITE(dingding)
BOOST_AUTO_TEST_CASE(access_token) {
  doodle_lib l_lib{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};
  auto l_c = std::make_shared<doodle::dingding::client>(l_ctx, "https://api.dingtalk.com/", "443");
}

BOOST_AUTO_TEST_SUITE_END()