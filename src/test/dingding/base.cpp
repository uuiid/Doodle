//
// Created by TD on 2022/9/8.
//

#include <doodle_core/doodle_core.h>

#include <doodle_dingding/client/client.h>
#include <doodle_dingding/client/dingding_api.h>
#include <doodle_dingding/fmt_lib/boost_beast_fmt.h>
#include <doodle_dingding/metadata/attendance.h>
#include <doodle_dingding/metadata/request_base.h>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include <main_fixtures/lib_fixtures.h>
using namespace doodle;

struct loop_fixtures : lib_fixtures {
  loop_fixtures()  = default;
  ~loop_fixtures() = default;
  boost::asio::ssl::context context_attr{boost::asio::ssl::context::sslv23};
  void setup() {
    context_attr.set_verify_mode(boost::asio::ssl::verify_peer);
    context_attr.set_options(
        boost::asio::ssl::context::default_workarounds | boost::asio::ssl::context::no_sslv2 |
        boost::asio::ssl::context::no_sslv3
    );
    context_attr.set_default_verify_paths();
    BOOST_TEST_MESSAGE("完成夹具设置");
  };
  void teardown(){};
};

BOOST_FIXTURE_TEST_CASE(client_base_tset, loop_fixtures) {
  using namespace std::literals;

  auto l_r            = std::make_shared<dingding::client>(boost::asio::make_strand(g_io_context()), context_attr);
  using request_type  = boost::beast::http::request<boost::beast::http::empty_body>;
  using response_type = boost::beast::http::response<boost::beast::http::string_body>;
  request_type l_req{};
  l_req.method(boost::beast::http::verb::get);
  l_req.keep_alive(false);
  boost::url l_url{"https://www.baidu.com/"s};
  l_r->async_write_read<response_type>(l_req, l_url, [](boost::system::error_code, const response_type& in) {
    DOODLE_LOG_INFO(in);
  });
  g_io_context().run();
}

BOOST_FIXTURE_TEST_SUITE(dingding_base, loop_fixtures)
using namespace entt::literals;

using globe_department_id = entt::monostate<"globe_department_id"_hs>;
using globe_user_id       = entt::monostate<"globe_user_id"_hs>;

BOOST_AUTO_TEST_CASE(client_find_user_by_mobile) {
  using namespace std::literals;

  auto l_st = boost::asio::make_strand(g_io_context());
  auto l_c  = std::make_shared<dingding::dingding_api>(l_st, context_attr);

  l_c->async_find_mobile_user(
      "15825515923"s,
      [=](const boost::system::error_code& in_err, const dingding::user_dd& in_u) {
        BOOST_TEST(!in_err);
        BOOST_TEST(!in_u.name.empty());
        DOODLE_LOG_INFO(in_u.userid);
        globe_user_id{} = in_u.userid;
        BOOST_TEST(!in_u.dept_id_list.empty());
        DOODLE_LOG_INFO(fmt::to_string(in_u.dept_id_list));
        globe_department_id{} = in_u.dept_id_list.back();
      }
  );
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(client_get_dep, *boost::unit_test::depends_on("dingding_base/client_find_user_by_mobile")) {
  using namespace std::literals;
  auto l_st          = boost::asio::make_strand(g_io_context());
  auto l_c           = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  std::int32_t l_dep = globe_department_id{};
  l_c->async_get_departments(l_dep, [=](const boost::system::error_code& in_err, const dingding::department& in_dep) {
    BOOST_TEST(!in_err);
    DOODLE_LOG_INFO(in_dep.name);
    DOODLE_LOG_INFO(in_dep.dept_id);
  });
  g_io_context().run();
}

BOOST_AUTO_TEST_CASE(
    client_get_user_attendance2, *boost::unit_test::depends_on("dingding_base/client_find_user_by_mobile")
) {
  using namespace std::literals;
  auto l_st = boost::asio::make_strand(g_io_context());
  auto l_c  = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  time_point_wrap l_end_day{};
  time_point_wrap l_begin_day{l_end_day - doodle::chrono::days{6}};
  std::string l_user_id = globe_user_id{};

  bool is_run_chick{};
  l_c->async_get_user_updatedata_attendance(
      l_begin_day, l_user_id,
      [=, l_run = &is_run_chick](
          const boost::system::error_code& in_err, const dingding::attendance::attendance& in_att
      ) mutable {
        BOOST_TEST(!in_err);
        *l_run = true;
      }
  );
  g_io_context().run();
  BOOST_TEST(is_run_chick);
}
BOOST_AUTO_TEST_CASE(
    client_get_user_attendance3, *boost::unit_test::depends_on("dingding_base/client_find_user_by_mobile")
) {
  using namespace std::literals;
  auto l_st = boost::asio::make_strand(g_io_context());
  auto l_c  = std::make_shared<dingding::dingding_api>(l_st, context_attr);
  time_point_wrap l_end_day{};
  time_point_wrap l_begin_day{l_end_day - doodle::chrono::days{30}};
  std::string l_user_id = globe_user_id{};

  bool is_run_chick{};
  l_c->async_get_user_updatedata_attendance_list(
      l_begin_day, l_end_day, l_user_id,
      [=, l_run = &is_run_chick](
          const boost::system::error_code& in_err, const std::vector<dingding::attendance::attendance>& in_list
      ) mutable {
        BOOST_TEST(!in_err);
        nlohmann::json l_json{};
        l_json = in_list;
        DOODLE_LOG_INFO(l_json.dump());

        *l_run = true;
      }
  );
  g_io_context().run();
  BOOST_TEST(is_run_chick);
}
BOOST_AUTO_TEST_SUITE_END()
