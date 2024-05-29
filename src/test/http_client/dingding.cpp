#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>

#include <doodle_lib/http_client/dingding_client.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;
BOOST_AUTO_TEST_SUITE(dingding)
BOOST_AUTO_TEST_CASE(access_token) {
  app_base l_app_base{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};

  auto l_env               = boost::this_process::environment();
  std::string l_app_key    = l_env["DINGDING_APP_KEY"].to_string();
  std::string l_app_secret = l_env["DINGDING_APP_SECRET"].to_string();

  auto l_c                 = std::make_shared<doodle::dingding::client>(l_ctx);

  l_c->access_token(l_app_key, l_app_secret, false, [](auto ec, auto json) {
    BOOST_TEST(!ec);
    BOOST_TEST(json.contains("accessToken"));
    BOOST_TEST(json.contains("expireIn"));
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_user_by_mobile) {
  app_base l_app_base{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};

  auto l_env               = boost::this_process::environment();
  std::string l_app_key    = l_env["DINGDING_APP_KEY"].to_string();
  std::string l_app_secret = l_env["DINGDING_APP_SECRET"].to_string();
  std::string l_mobile     = l_env["DINGDING_MOBILE"].to_string();

  auto l_c                 = std::make_shared<doodle::dingding::client>(l_ctx);

  l_c->access_token(l_app_key, l_app_secret, false, [l_c, l_mobile](auto ec, auto json) {
    BOOST_TEST(!ec);
    BOOST_TEST(json.contains("accessToken"));
    BOOST_TEST(json.contains("expireIn"));

    l_c->get_user_by_mobile(l_mobile, [l_c](auto ec, auto json) {
      BOOST_TEST(!ec);
      default_logger_raw()->info("json: {}", json.dump());
      BOOST_TEST(json.contains("result"));
      BOOST_TEST(json["result"].contains("userid"));
      default_logger_raw()->info("userid: {}", json["result"]["userid"].get<std::string>());
    });
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_attendance_updatedata) {
  app_base l_app_base{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};

  auto l_env               = boost::this_process::environment();
  std::string l_app_key    = l_env["DINGDING_APP_KEY"].to_string();
  std::string l_app_secret = l_env["DINGDING_APP_SECRET"].to_string();
  std::string l_mobile     = l_env["DINGDING_MOBILE"].to_string();

  auto l_c                 = std::make_shared<doodle::dingding::client>(l_ctx);

  l_c->access_token(l_app_key, l_app_secret, false, [l_c, l_mobile](auto ec, auto json) {
    BOOST_TEST(!ec);
    BOOST_TEST(json.contains("accessToken"));
    BOOST_TEST(json.contains("expireIn"));

    l_c->get_attendance_updatedata("286954280924345155", time_point_wrap{2024, 5, 23}, [l_c](auto ec, auto json) {
      BOOST_TEST(!ec);
      default_logger_raw()->info("json: {}", json.dump());
      BOOST_TEST(json.contains("result"));
      BOOST_TEST(json["result"].contains("approve_list"));
    });
  });
  g_io_context().run();
}
BOOST_AUTO_TEST_SUITE_END()