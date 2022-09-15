//
// Created by TD on 2022/9/8.
//
#define BOOST_TEST_MODULE dingding
#include <boost/test/included/unit_test.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/data/monomorphic.hpp>

#include <doodle_dingding/client/client.h>
#include <doodle_dingding/client/dingding_api.h>
#include <doodle_dingding/metadata/access_token.h>
#include <doodle_dingding/metadata/request_base.h>

#include <doodle_core/doodle_core.h>
#include <doodle_dingding/fmt_lib/boost_beast_fmt.h>

#include <boost/asio/ssl.hpp>
#include <boost/asio.hpp>
using namespace doodle;

struct loop_fixtures {
  loop_fixtures()  = default;
  ~loop_fixtures() = default;
  boost::asio::io_context io_context_attr{};
  boost::asio::ssl::context context_attr{
      boost::asio::ssl::context::sslv23};
  doodle_lib l_lib{};
  void setup() {
    context_attr.set_verify_mode(boost::asio::ssl::verify_peer);
    context_attr.set_options(
        boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3
    );
    context_attr.set_default_verify_paths();
    BOOST_TEST_MESSAGE("完成夹具设置");
  };
  void teardown() {
    BOOST_TEST_MESSAGE("开始运行");
    io_context_attr.run();
  };
};

BOOST_FIXTURE_TEST_CASE(client_base_tset, loop_fixtures) {
  using namespace std::literals;

  auto l_r = std::make_shared<dingding::client>(
      boost::asio::make_strand(io_context_attr), context_attr
  );
  using request_type  = boost::beast::http::request<boost::beast::http::empty_body>;
  using response_type = boost::beast::http::response<boost::beast::http::string_body>;
  request_type l_req{};
  l_req.method(boost::beast::http::verb::get);
  l_req.keep_alive(false);
  boost::url l_url{"https://www.baidu.com/"s};
  l_r->async_write_read<response_type>(
      l_req,
      l_url,
      [](
          boost::system::error_code,
          const response_type& in
      ) {
        DOODLE_LOG_INFO(in);
      }
  );
}

BOOST_FIXTURE_TEST_SUITE(dingding_base, loop_fixtures)
using namespace entt::literals;

using globe_access_token = entt::monostate<"globe_access_token"_hs>;
using globe_department   = entt::monostate<"globe_department"_hs>;

BOOST_AUTO_TEST_CASE(client_get_gettoken) {
  using namespace std::literals;
  std::make_shared<dingding::dingding_api>(boost::asio::make_strand(io_context_attr), context_attr)
      ->async_get_token([](const dingding::access_token& in) {
        globe_access_token{} = in;
        DOODLE_LOG_INFO(in.token);
      });
}

BOOST_AUTO_TEST_CASE(client_get_dep) {
  using namespace std::literals;
  auto l_st                      = boost::asio::make_strand(io_context_attr);
  auto l_c                       = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  dingding::access_token l_token = globe_access_token{};
  l_c->async_get_departments(
      dingding::department_query{145552127, "zh_CN"s},
      l_token,
      [=](const std::vector<entt::handle>& in_handle) {
        ranges::for_each(
            in_handle, [](auto&& i) {
              auto l_dep         = i.get<dingding::department>();
              globe_department{} = l_dep;
              BOOST_TEST(i.any_of<dingding::department>());
              BOOST_TEST_MESSAGE(l_dep.name);
            }
        );
      }
  );
}

BOOST_AUTO_TEST_CASE(client_get_dep_user) {
  using namespace std::literals;

  auto l_st                      = boost::asio::make_strand(io_context_attr);
  auto l_c                       = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  dingding::access_token l_token = globe_access_token{};
  dingding::department l_dep     = globe_department{};

  l_c->async_get_departments_user(
      dingding::user_dd_ns::dep_query{
          l_dep.dept_id,
          0,
          50},
      l_token,
      [=](const std::vector<entt::handle>& in_handle) {
        ranges::for_each(
            in_handle, [](auto&& i) {
              BOOST_TEST(i.any_of<dingding::user_dd>());
              BOOST_TEST_MESSAGE(i.get<dingding::user_dd>().name);
              BOOST_TEST_MESSAGE(i.get<dingding::user_dd>().userid);
            }
        );
      }
  );
}

BOOST_AUTO_TEST_SUITE_END()
