#include <doodle_core/core/doodle_lib.h>

#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

BOOST_AUTO_TEST_SUITE(kitsu)
constexpr static std::string_view g_token{
    "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9."
    "eyJmcmVzaCI6ZmFsc2UsImlhdCI6MTcxNjc5NDgzNywianRpIjoiMzkxNTFiNjEtNzJkZC00MGE3LTk3MmMtMGJhYWIzMmUwMzY4IiwidHlwZSI6Im"
    "FjY2VzcyIsInN1YiI6ImU5OWMyNjZhLTk1ZjUtNDJmNS1hYmUxLWI0MTlkMjk4MmFiMCIsIm5iZiI6MTcxNjc5NDgzNywiZXhwIjoxNzY0NjMzNjAw"
    "LCJpZGVudGl0eV90eXBlIjoiYm90In0.sYmTTWsvMNGLiX57_RnfUI9rL04WJL2XSNZElSL4RNY"
};

BOOST_AUTO_TEST_CASE(login) {
  doodle_lib l_lib{};
  auto l_c = doodle::kitsu::kitsu_client{"192.168.40.182", "80"};
  l_c.longin("957714080@qq.com", "8jO6sJm5EYAZSuZ7wy3P", [](boost::system::error_code ec, nlohmann::json in_json) {
    BOOST_TEST(!ec);
    BOOST_TEST_MESSAGE(in_json.dump());
    BOOST_TEST(in_json["login"].get<bool>());
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(authenticated) {
  doodle_lib l_lib{};
  auto l_c = doodle::kitsu::kitsu_client{"192.168.40.182", "80"};
  l_c.authenticated(std::string{g_token}, [](boost::system::error_code ec, nlohmann::json in_json) {
    BOOST_TEST(!ec);
    BOOST_TEST_MESSAGE(in_json.dump());
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(get_task) {
  doodle_lib l_lib{};
  auto l_c = std::make_shared<doodle::kitsu::kitsu_client>("192.168.40.182", "80");
  l_c->longin("957714080@qq.com", "8jO6sJm5EYAZSuZ7wy3P", [l_c](boost::system::error_code ec, nlohmann::json in_json) {
    BOOST_TEST(!ec);
    BOOST_TEST_MESSAGE(in_json.dump());
    BOOST_TEST(in_json["login"].get<bool>());

    l_c->get_task("e5ece525-3e5b-4c71-be85-32e32d336e4e", [](boost::system::error_code ec, nlohmann::json in_json) {
      BOOST_TEST(!ec);
      BOOST_TEST_MESSAGE(in_json.dump());
    });
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_SUITE_END()