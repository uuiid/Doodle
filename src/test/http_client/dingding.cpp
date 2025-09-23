#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>

#include <doodle_lib/http_client/dingding_client.h>

#include <boost/process/v2/environment.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;
BOOST_AUTO_TEST_SUITE(dingding)

BOOST_AUTO_TEST_CASE(access_token) {
  app_base l_app_base{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};

  auto l_env = boost::process::v2::environment::current();

  std::string l_app_key{};
  std::string l_app_secret{};
  for (auto&& l_it : l_env) {
    if (l_it.key() == L"DINGDING_APP_KEY") l_app_key = l_it.value().string();
    if (l_it.key() == L"DINGDING_APP_SECRET") l_app_secret = l_it.value().string();
  }
  std::string l_mobile = std::string{};
  BOOST_TEST(!l_mobile.empty());

  auto l_c          = std::make_shared<doodle::dingding::client>(l_ctx);
  auto l_work_guard = boost::asio::make_work_guard(g_io_context());
  l_c->access_token(l_app_key, l_app_secret);

  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_user_by_mobile) {
  app_base l_app_base{};

  // l_c->access_token(l_app_key, l_app_secret, false);
  auto l_f = boost::asio::co_spawn(
      g_io_context(),
      []() -> boost::asio::awaitable<void> {
        boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};

        auto l_env = boost::process::v2::environment::current();
        std::string l_app_key{};
        std::string l_app_secret{};
        for (auto&& l_it : l_env) {
          if (l_it.key() == L"DINGDING_APP_KEY") l_app_key = l_it.value().string();
          if (l_it.key() == L"DINGDING_APP_SECRET") l_app_secret = l_it.value().string();
        }
        std::string l_mobile = std::string{};
        BOOST_TEST(!l_mobile.empty());

        auto l_c = std::make_shared<doodle::dingding::client>(l_ctx);

        l_c->access_token(l_app_key, l_app_secret);
        auto l_r = co_await l_c->get_user_by_mobile(l_mobile);
        app_base::Get().on_cancel.emit();
      }(),
      boost::asio::use_future
  );

  g_io_context().run();
  // auto [l_e, l_r] = l_f.get();

  // BOOST_TEST(!l_e);
  // BOOST_TEST_MESSAGE(l_r);
}

BOOST_AUTO_TEST_CASE(get_attendance_updatedata) {
  app_base l_app_base{};

  auto l_f = boost::asio::co_spawn(
      g_io_context(),
      []() -> boost::asio::awaitable<void> {
        boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};

        auto l_env = boost::process::v2::environment::current();
        std::string l_app_key{};
        std::string l_app_secret{};
        for (auto&& l_it : l_env) {
          if (l_it.key() == L"DINGDING_APP_KEY") l_app_key = l_it.value().string();
          if (l_it.key() == L"DINGDING_APP_SECRET") l_app_secret = l_it.value().string();
        }
        std::string l_time_str = "2024-05-10";
        std::string l_user_id  = {};
        chrono::local_time_pos l_time{};
        std::istringstream l_iss{l_time_str};
        l_iss >> chrono::parse("%Y-%m-%d", l_time);

        auto l_c = std::make_shared<doodle::dingding::client>(l_ctx);

        l_c->access_token(l_app_key, l_app_secret);
        auto l_r = co_await l_c->get_attendance_updatedata(l_user_id, l_time);
        co_return;
      }(),
      boost::asio::use_future
  );

  g_io_context().run();
  // auto [l_e] = l_f.get();

  // BOOST_TEST(!l_e);
  // BOOST_TEST_MESSAGE(l_r);
}
BOOST_AUTO_TEST_SUITE_END()