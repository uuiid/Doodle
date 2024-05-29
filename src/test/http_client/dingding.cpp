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

  auto l_env        = boost::this_process::environment();
  std::string l_app_key    = l_env["DINGDING_APP_KEY"].to_string();
  std::string l_app_secret = l_env["DINGDING_APP_SECRET"].to_string();

  auto l_c          = std::make_shared<doodle::dingding::client>(l_ctx, "https://api.dingtalk.com/", "443");

  l_c->access_token(l_app_key, l_app_secret, [](auto ec, auto json) {
    BOOST_TEST(!ec);
    BOOST_TEST(json.contains("access_token"));
    BOOST_TEST(json.contains("expires_in"));
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_SUITE_END()