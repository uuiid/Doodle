//
// Created by TD on 2022/9/8.
//
#define BOOST_TEST_MODULE dingding
#include <boost/test/included/unit_test.hpp>
#include <doodle_dingding/client/client.h>
#include <doodle_dingding/client/dingding_api.h>
#include <doodle_dingding/metadata/access_token.h>
#include <doodle_dingding/metadata/department.h>

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

  dingding::client_ns::http_req_res<
      boost::beast::http::request<boost::beast::http::empty_body>,
      boost::beast::http::response<boost::beast::http::string_body>>
      l_http_req_res{l_r->shared_from_this()};
  l_http_req_res.req_attr.method(boost::beast::http::verb::get);
  l_http_req_res.url_attr = boost::url{"https://www.baidu.com/"s};
  l_http_req_res.req_attr.keep_alive(false);
  l_http_req_res.read_fun = [](auto&& in) {
    DOODLE_LOG_INFO(in);
  };
  l_r->run(l_http_req_res);
}

BOOST_FIXTURE_TEST_SUITE(dingding_base, loop_fixtures)
static dingding::access_token l_token{};

BOOST_AUTO_TEST_CASE(client_get_gettoken) {
  using namespace std::literals;
  std::make_shared<dingding::dingding_api>(boost::asio::make_strand(io_context_attr), context_attr)
      ->async_get_token([](const dingding::access_token& in) {
        DOODLE_LOG_INFO(in.token);
        l_token = in;
      });
}

BOOST_AUTO_TEST_CASE(client_get_dep) {
  using namespace std::literals;

  auto l_st = boost::asio::make_strand(io_context_attr);
  auto l_c  = std::make_shared<dingding::dingding_api>(l_st, context_attr);

  std::shared_ptr<std::function<void(const dingding::department_query&)>>
      l_call_ptr{std::make_shared<std::function<void(const dingding::department_query&)>>()};
  *l_call_ptr = [l_c, l_st, l_call_ptr](const dingding::department_query& in_q) {
    l_c->async_get_departments(dingding::department_query{}, l_token, [=](const entt::handle& in_handle) {
      BOOST_TEST(in_handle.any_of<dingding::department>());
      DOODLE_LOG_INFO(in_handle.get<dingding::department>().name);
      boost::asio::post(l_st, [=]() { (*l_call_ptr)(
                                          dingding::department_query{
                                              in_handle.get<dingding::department>().dept_id, "zh_CN"s}
                                      ); });
    });
  };
  (*l_call_ptr)(dingding::department_query{});


}

BOOST_AUTO_TEST_SUITE_END()
