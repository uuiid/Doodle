//
// Created by td_main on 2023/8/15.
//

#include "doodle_core/core/core_help_impl.h"
#include "doodle_core/database_task/sqlite_client.h"
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/comment.h>
#include <doodle_core/metadata/episodes.h>
#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/metadata/image_icon.h>
#include <doodle_core/metadata/importance.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/move_create.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/redirection_path_info.h>
#include <doodle_core/metadata/rules.h>
#include <doodle_core/metadata/season.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/time_point_wrap.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>
#include <doodle_core/pin_yin/convert.h>

#include "doodle_app/app/app_command.h"

#include <doodle_lib/render_farm/detail/basic_json_body.h>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/test/unit_test.hpp>
using namespace doodle;
namespace {
struct http_server {
  doodle_lib lib{};
  boost::beast::tcp_stream stream_{boost::asio::make_strand(g_io_context())};
  http_server() {
    boost::asio::ip::tcp::resolver l_resolver{boost::asio::make_strand(g_io_context())};
    auto l_results = l_resolver.resolve("127.0.0.1", std::to_string(doodle_config::http_port));
    stream_.connect(l_results);
  }
  inline static std::int32_t id;
};
}  // namespace
BOOST_FIXTURE_TEST_SUITE(server, http_server)

BOOST_AUTO_TEST_SUITE(computer)

BOOST_AUTO_TEST_CASE(post_) {
  boost::beast::http::request<boost::beast::http::string_body> l_request{
      boost::beast::http::verb::post, "/v1/render_farm/computer", 11};

  l_request.body() = fmt::format(R"({{"name": "{}"}})", boost::asio::ip::host_name());
  l_request.keep_alive(false);
  l_request.prepare_payload();
  boost::beast::http::write(stream_, l_request);
  boost::beast::flat_buffer l_buffer;
  boost::beast::http::response<render_farm::detail::basic_json_body> l_response;
  boost::beast::http::read(stream_, l_buffer, l_response);
  BOOST_TEST(l_response.result() == boost::beast::http::status::ok);
  auto l_body = l_response.body();
  BOOST_TEST_MESSAGE(l_body.dump());
}

BOOST_AUTO_TEST_CASE(post_error) {
  boost::beast::http::request<boost::beast::http::string_body> l_request{
      boost::beast::http::verb::post, "/v1/render_farm/computer", 11};

  l_request.body() = R"({{"name": 1233}})";
  l_request.keep_alive(false);
  boost::beast::http::write(stream_, l_request);
  boost::beast::flat_buffer l_buffer;
  boost::beast::http::response<boost::beast::http::string_body> l_response;
  boost::beast::http::read(stream_, l_buffer, l_response);
  BOOST_TEST(l_response.result() != boost::beast::http::status::ok);
  BOOST_TEST_MESSAGE(l_response.body());
}

BOOST_AUTO_TEST_CASE(get_) {
  boost::beast::http::request<boost::beast::http::empty_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm/computer", 11};
  l_request.keep_alive(false);
  boost::beast::http::write(stream_, l_request);
  boost::beast::flat_buffer l_buffer;
  boost::beast::http::response<boost::beast::http::string_body> l_response;
  boost::beast::http::read(stream_, l_buffer, l_response);
  BOOST_TEST(l_response.result() == boost::beast::http::status::ok);
  BOOST_TEST_MESSAGE(l_response.body());

  auto l_json = nlohmann::json::parse(l_response.body());
  //  auto l_id_list = l_json.get<std::vector<std::int32_t>>();
  //
  //  auto l_it      = std::find(l_id_list.begin(), l_id_list.end(), id);
  //  BOOST_TEST((l_it != l_id_list.end()));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(render_job)

BOOST_AUTO_TEST_CASE(post) {
  boost::beast::http::request<boost::beast::http::string_body> l_request{
      boost::beast::http::verb::post, "/v1/render_farm/render_job", 11};
  auto l_json = nlohmann::json::parse(
      R"(
{
    "projectPath": "occaecat Excepteur sint quis laborum",
    "args": "ut tempor",
    "manifestValue": "voluptate ut veniam",
    "outFilePath": "id occaecat aute nisi elit"
}
)"
  );

  l_request.body() = l_json.dump();
  l_request.keep_alive(false);
  l_request.prepare_payload();
  boost::beast::http::write(stream_, l_request);
  boost::beast::flat_buffer l_buffer;
  boost::beast::http::response<render_farm::detail::basic_json_body> l_response;
  boost::beast::http::read(stream_, l_buffer, l_response);
  BOOST_TEST(l_response.result() == boost::beast::http::status::ok);
  auto l_body = l_response.body();
  id          = l_body["id"].get<std::int32_t>();
  BOOST_TEST_MESSAGE(l_body.dump());
}

BOOST_AUTO_TEST_CASE(get_log, *boost::unit_test::depends_on("server/render_job/post")) {
  boost::beast::http::request<boost::beast::http::empty_body> l_request{
      boost::beast::http::verb::get, fmt::format("/v1/render_farm/get_log/{}", id), 11};
  l_request.keep_alive(false);
  boost::beast::http::write(stream_, l_request);
  boost::beast::flat_buffer l_buffer;
  boost::beast::http::response<boost::beast::http::string_body> l_response;
  boost::beast::http::read(stream_, l_buffer, l_response);
  BOOST_TEST(l_response.result() == boost::beast::http::status::ok);
  BOOST_TEST_MESSAGE(l_response.body());

  //  auto l_id_list = l_json.get<std::vector<std::int32_t>>();
  //
  //  auto l_it      = std::find(l_id_list.begin(), l_id_list.end(), id);
  //  BOOST_TEST((l_it != l_id_list.end()));
}
BOOST_AUTO_TEST_CASE(get_error, *boost::unit_test::depends_on("server/render_job/post")) {
  boost::beast::http::request<boost::beast::http::empty_body> l_request{
      boost::beast::http::verb::get, fmt::format("/v1/render_farm/get_err/{}", id), 11};
  l_request.keep_alive(false);
  boost::beast::http::write(stream_, l_request);
  boost::beast::flat_buffer l_buffer;
  boost::beast::http::response<boost::beast::http::string_body> l_response;
  boost::beast::http::read(stream_, l_buffer, l_response);
  BOOST_TEST(l_response.result() == boost::beast::http::status::ok);
  BOOST_TEST_MESSAGE(l_response.body());

  //  auto l_id_list = l_json.get<std::vector<std::int32_t>>();
  //
  //  auto l_it      = std::find(l_id_list.begin(), l_id_list.end(), id);
  //  BOOST_TEST((l_it != l_id_list.end()));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(client)

BOOST_AUTO_TEST_CASE(forward_to_server) {
  boost::beast::http::request<boost::beast::http::string_body> l_request{
      boost::beast::http::verb::get, "/v1/render_farm/client_submit_job", 11};

  auto l_json = nlohmann::json::parse(
      R"(
{
    "projectPath": "occaecat Excepteur sint quis laborum",
    "args": "ut tempor",
    "manifestValue": "voluptate ut veniam",
    "outFilePath": "id occaecat aute nisi elit"
}
)"
  );

  l_request.body() = l_json.dump();

  l_request.keep_alive(false);
  boost::beast::http::write(stream_, l_request);
  boost::beast::flat_buffer l_buffer;
  boost::beast::http::response<boost::beast::http::string_body> l_response;
  boost::beast::http::read(stream_, l_buffer, l_response);
  BOOST_TEST(l_response.result() == boost::beast::http::status::ok);
  BOOST_TEST_MESSAGE(l_response.body());

  auto l_res_json = nlohmann::json::parse(l_response.body());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE_END()
