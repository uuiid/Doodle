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
  l_app_base.use_multithread(true);
  boost::asio::ssl::context l_ctx{boost::asio::ssl::context::tlsv12_client};
  auto l_c = std::make_shared<http::detail::http_client_data_base>(g_io_context());
  l_c->init("https://www.baidu.com/", &l_ctx);
  boost::beast::http::request<boost::beast::http::empty_body> l_req{boost::beast::http::verb::get, "/", 11};
  l_req.keep_alive(true);
  l_req.prepare_payload();
  std::vector<decltype(boost::asio::co_spawn(
      g_io_context(), http::detail::read_and_write<boost::beast::http::string_body>(l_c, l_req), boost::asio::use_future
  ))>
      l_vs{};
  std::shared_ptr<std::atomic_int32_t> l_p = std::make_shared<std::atomic_int32_t>(0);
  for (int i = 0; i < 100; ++i) {
    boost::asio::co_spawn(
        g_io_context(), http::detail::read_and_write<boost::beast::http::string_body>(l_c, l_req),
        [l_p, l_c](
            std::exception_ptr e,
            std::tuple<boost::system::error_code, boost::beast::http::response<boost::beast::http::string_body>> res
        ) {
          auto [l_ex, l_] = res;
          BOOST_TEST(!l_ex);
          BOOST_TEST_MESSAGE(l_ex.message());
          ++*l_p;
          if (*l_p == 100) {
            l_c->do_close();
            app_base::Get().stop_app();
          }
        }
    );
  }

  l_app_base.run();
}

BOOST_AUTO_TEST_SUITE_END()