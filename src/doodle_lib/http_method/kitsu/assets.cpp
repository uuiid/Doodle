//
// Created by TD on 24-12-30.
//

#include "assets.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> assets_new(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  auto l_c     = create_kitsu_proxy(in_handle);

  auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_c, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }
  co_return std::move(l_res);
}

DOODLE_HTTP_FUN(with_tasks, get, "api/data/assets/with-tasks", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override {
  get_person(in_handle);
  uuid l_prj_id{};
  if (auto l_str = in_handle->capture_->get("project_id"); !l_str.empty()) l_prj_id = from_uuid_str(l_str);
}
DOODLE_HTTP_FUN_END()

}  // namespace
void assets_reg2(http_route& in_http_route) {
  in_http_route
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/data/projects/{project_id}/asset-types/{asset_type_id}/assets/new",
              assets_new
          )
      )
#ifdef DOODLE_KITSU
      .reg(std::make_shared<with_tasks_get>())
#endif
      ;
}
}  // namespace doodle::http::kitsu