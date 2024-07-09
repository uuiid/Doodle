#include <doodle_core/core/doodle_lib.h>

#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

BOOST_AUTO_TEST_SUITE(kitsu)
constexpr static std::string_view g_token{
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJmcmVzaCI6ZmFsc2UsImlhdCI6MTcxNzU1MDUxMywianRpIjoiOTU0MDg1NjctMzE1OS00Y2MzLTljM2ItZmNiMzQ4MTIwNjU5IiwidHlwZSI6Im"
    "FjY2VzcyIsInN1YiI6ImU5OWMyNjZhLTk1ZjUtNDJmNS1hYmUxLWI0MTlkMjk4MmFiMCIsIm5iZiI6MTcxNzU1MDUxMywiZXhwIjoxNzY0NjMzNjAw"
    "LCJpZGVudGl0eV90eXBlIjoiYm90In0.xLV17bMK8VH0qavV4Ttbi43RhaBqpc1LtTUbRwu1684"
};

BOOST_AUTO_TEST_CASE(authenticated) {
  doodle_lib l_lib{};
  auto l_c = doodle::kitsu::kitsu_client{"192.168.40.182", "80"};
  l_c.authenticated(std::string{g_token}, [](boost::system::error_code ec, nlohmann::json in_json) {
    BOOST_TEST(!ec);
    BOOST_TEST_MESSAGE(in_json.dump());
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(authenticated2) {
  doodle_lib l_lib{};
  auto l_c = std::make_shared<doodle::http::detail::http_client_data_base>(boost::asio::make_strand(g_io_context()));
  l_c->init("http://192.168.40.182");
  boost::beast::http::request<boost::beast::http::string_body> req{
      boost::beast::http::verb::get, "/api/auth/authenticated", 11
  };
  req.set(boost::beast::http::field::host, "192.168.40.182");
  req.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
  req.set(boost::beast::http::field::authorization, "Bearer " + std::string{g_token});
  boost::asio::co_spawn(
      g_io_context(), doodle::http::detail::read_and_write<boost::beast::http::string_body>(l_c, req),
      boost::asio::bind_executor(g_io_context(), boost::asio::use_future)
  );
  g_io_context().run();
  
}

BOOST_AUTO_TEST_CASE(get_task) {
  doodle_lib l_lib{};
  auto l_c = std::make_shared<doodle::kitsu::kitsu_client>("192.168.40.182", "80");
  l_c->authenticated(std::string{g_token}, [l_c](boost::system::error_code ec, nlohmann::json in_json) {
    BOOST_TEST(!ec);
    BOOST_TEST_MESSAGE(in_json.dump());

    l_c->get_task("cde5305c-678c-4a3d-8baf-79cddfc9e9c3", [](boost::system::error_code ec, nlohmann::json in_json) {
      BOOST_TEST(!ec);
      default_logger_raw()->log(log_loc(), level::info, in_json.dump());
      BOOST_TEST_MESSAGE(in_json.dump());
    });
  });
  g_io_context().run();
}

// 直接获取task
BOOST_AUTO_TEST_CASE(get_task_directly) {
  doodle_lib l_lib{};
  auto l_c = std::make_shared<doodle::kitsu::kitsu_client>("192.168.40.182", "80");
  l_c->set_access_token(std::string{g_token});
  l_c->get_task("cde5305c-678c-4a3d-8baf-79cddfc9e9c3", [](boost::system::error_code ec, nlohmann::json in_json) {
    BOOST_TEST(!ec);
    default_logger_raw()->log(log_loc(), level::info, in_json.dump());
    BOOST_TEST_MESSAGE(in_json.dump());
  });

  g_io_context().run();
}

// 获取用户
BOOST_AUTO_TEST_CASE(get_user) {
  doodle_lib l_lib{};
  auto l_c = std::make_shared<doodle::kitsu::kitsu_client>("192.168.40.182", "80");
  l_c->set_access_token(std::string{g_token});
  l_c->get_user(
      boost::lexical_cast<boost::uuids::uuid>("69a8d093-dcab-4890-8f9d-c51ef065d03b"),
      [](boost::system::error_code ec, nlohmann::json in_json) {
        BOOST_TEST(!ec);
        default_logger_raw()->log(log_loc(), level::info, in_json.dump());
        BOOST_TEST_MESSAGE(in_json.dump());
      }
  );

  g_io_context().run();
}

BOOST_AUTO_TEST_SUITE_END()