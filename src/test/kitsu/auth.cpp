#include <doodle_core/core/doodle_lib.h>

#include <doodle_lib/http_client/kitsu_client.h>

#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>
using namespace doodle;

BOOST_AUTO_TEST_CASE(kitsu_login) {
  doodle_lib l_lib{};
  auto l_c = doodle::kitsu::kitsu_client{"192.168.40.182", "80"};
  l_c.longin("957714080@qq.com", "8jO6sJm5EYAZSuZ7wy3P", [](boost::system::error_code ec, nlohmann::json in_json) {
    BOOST_TEST(!ec);
    BOOST_TEST_MESSAGE(in_json.dump());
    BOOST_TEST(in_json["login"].get<bool>());
  });
  g_io_context().run();
}