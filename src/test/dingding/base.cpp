//
// Created by TD on 2022/9/8.
//

#include <main_fixtures/lib_fixtures.h>

#define BOOST_TEST_MODULE dingding
#define BOOST_TEST_DYN_LINK

#include <boost/test/unit_test.hpp>
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

struct loop_fixtures : lib_fixtures {
  loop_fixtures()  = default;
  ~loop_fixtures() = default;
  boost::asio::io_context io_context_attr{};
  boost::asio::ssl::context context_attr{
      boost::asio::ssl::context::sslv23};
  void setup() {
    doodle_lib::create_time_database();
    context_attr.set_verify_mode(boost::asio::ssl::verify_peer);
    context_attr.set_options(
        boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 | boost::asio::ssl::context::no_sslv3
    );
    context_attr.set_default_verify_paths();
    BOOST_TEST_MESSAGE("完成夹具设置");
  };
  void teardown(){};
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
  io_context_attr.run();
}

BOOST_FIXTURE_TEST_SUITE(dingding_base, loop_fixtures)
using namespace entt::literals;

using globe_access_token  = entt::monostate<"globe_access_token"_hs>;
using globe_department_id = entt::monostate<"globe_department_id"_hs>;
using globe_user_id       = entt::monostate<"globe_user_id"_hs>;

BOOST_AUTO_TEST_CASE(client_get_gettoken) {
  using namespace std::literals;
  std::make_shared<dingding::dingding_api>(boost::asio::make_strand(io_context_attr), context_attr)
      ->async_get_token([](const dingding::access_token& in) {
        globe_access_token{} = in;
        DOODLE_LOG_INFO(in.token);
      });
  io_context_attr.run();
}

BOOST_AUTO_TEST_CASE(
    client_find_user_by_mobile,
    *boost::unit_test::depends_on("dingding_base/client_get_gettoken")
) {
  using namespace std::literals;

  auto l_st                      = boost::asio::make_strand(io_context_attr);
  auto l_c                       = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  dingding::access_token l_token = globe_access_token{};

  l_c->async_find_mobile_user(
      dingding::user_dd_ns::find_by_mobile{
          "13891558584"},
      l_token,
      [=](const std::vector<entt::handle>& in_handle) {
        ranges::for_each(
            in_handle, [](const entt::handle& i) {
              BOOST_TEST(i.any_of<dingding::user_dd>());
              auto l_user = i.get<dingding::user_dd>();
              BOOST_TEST(!l_user.name.empty());
              DOODLE_LOG_INFO(l_user.userid);
              globe_user_id{} = l_user.userid;
              BOOST_TEST(!l_user.dept_id_list.empty());
              DOODLE_LOG_INFO(fmt::to_string(l_user.dept_id_list));
              globe_department_id{} = l_user.dept_id_list.back();
            }
        );
      }
  );
  io_context_attr.run();
}

BOOST_AUTO_TEST_CASE(
    client_get_dep,
    *boost::unit_test::depends_on("dingding_base/client_get_gettoken") *
        boost::unit_test::depends_on("dingding_base/client_find_user_by_mobile")
) {
  using namespace std::literals;
  auto l_st                      = boost::asio::make_strand(io_context_attr);
  auto l_c                       = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  dingding::access_token l_token = globe_access_token{};
  std::int32_t l_dep             = globe_department_id{};
  l_c->async_get_departments(
      dingding::department_ns::department_query{l_dep, "zh_CN"s},
      l_token,
      [=](const std::vector<entt::handle>& in_handle) {
        ranges::for_each(
            in_handle, [](auto&& i) {
              auto l_dep = i.get<dingding::department>();

              BOOST_TEST(i.any_of<dingding::department>());
              DOODLE_LOG_INFO(l_dep.name);
              DOODLE_LOG_INFO(l_dep.dept_id);
            }
        );
      }
  );
  io_context_attr.run();
}

BOOST_AUTO_TEST_CASE(
    client_get_user_attendance,
    *boost::unit_test::depends_on("dingding_base/client_get_gettoken") *
        boost::unit_test::depends_on("dingding_base/client_find_user_by_mobile")
) {
  using namespace std::literals;
  auto l_st                      = boost::asio::make_strand(io_context_attr);
  auto l_c                       = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  dingding::access_token l_token = globe_access_token{};

  time_point_wrap l_end_day{};
  time_point_wrap l_begin_day{l_end_day - doodle::chrono::days{6}};
  std::string l_u = globe_user_id{};
  std::vector<std::string> l_user_id{l_u};

  bool is_run_chick{};
  l_c->async_get_user_day_attendance(
      dingding::attendance::query::get_day_data{
          l_begin_day,
          l_end_day,
          l_user_id},
      l_token,
      [=, l_run = &is_run_chick](const std::vector<entt::handle>& in_handle) mutable {
        BOOST_TEST(!in_handle.empty());
        ranges::for_each(
            in_handle, [=](auto&& i) {
              BOOST_TEST(i.any_of<dingding::attendance::day_data>());
              auto l_day = i.get<dingding::attendance::day_data>();
              BOOST_TEST(i.any_of<dingding::attendance::day_data>());
              DOODLE_LOG_INFO("{}", l_day);
            }
        );
        *l_run = true;
      }
  );
  io_context_attr.run();
  BOOST_TEST(is_run_chick);
}

BOOST_AUTO_TEST_SUITE_END()
