//
// Created by TD on 24-7-15.
//
#include "doodle_core/core/global_function.h"
#include <doodle_core/core/app_base.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/core/http_client_core.h>

#include <boost/asio/use_awaitable.hpp>
#include <boost/test/tools/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_log.hpp>

#include <utility>

using namespace doodle;
BOOST_AUTO_TEST_SUITE(http_test)

class app_test : public app_base {
 public:
  app_test() : app_base() {}
  bool init() override {
    use_multithread(true);
    return true;
  }
};

BOOST_AUTO_TEST_CASE(quequ_t) {
  app_test l_app_base{};
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};
  auto l_c = std::make_shared<http::http_client_ssl>("https://www.baidu.com/", l_ctx);
  boost::beast::http::request<boost::beast::http::empty_body> l_req{boost::beast::http::verb::get, "/", 11};
  l_req.keep_alive(true);
  l_req.prepare_payload();
  boost::beast::http::response<boost::beast::http::string_body> l_res{};
  auto l_work                              = boost::asio::make_work_guard(g_io_context());
  std::shared_ptr<std::atomic_int32_t> l_p = std::make_shared<std::atomic_int32_t>(0);
  // for (int i = 0; i < 100; ++i) {
  l_c->read_and_write(l_req, l_res, [l_p, l_c, &l_res](boost::system::error_code in_ec) {
    if (in_ec) {
      BOOST_TEST_MESSAGE(in_ec.message());
      BOOST_TEST(false);
    } else {
      l_p->fetch_add(1);
      BOOST_TEST_MESSAGE(l_res.body());
      BOOST_TEST(true);
    }
  });
  // }

  l_app_base.run();
}

BOOST_AUTO_TEST_SUITE_END()