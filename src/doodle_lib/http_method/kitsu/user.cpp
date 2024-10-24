//
// Created by TD on 24-8-21.
//

#include "user.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {

boost::asio::awaitable<boost::beast::http::message_generator> user_authenticated(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }
  auto l_json = nlohmann::json::parse(l_res.body());
  try {
    auto l_user     = l_json["user"];
    auto l_user_id  = l_user["id"].get<uuid>();

    auto l_users    = g_ctx().get<sqlite_database>().get_by_uuid<user_helper::database_t>(l_user_id);
    auto l_user_ptr = std::make_shared<user_helper::database_t>();
    if (!l_users.empty()) *l_user_ptr = l_users.front();

    if (l_user_ptr->mobile_ != l_user["phone"].get<std::string>() ||
        l_user_ptr->power_ != l_user["role"].get<power_enum>()) {
      l_user_ptr->mobile_ = l_user["phone"].get<std::string>();
      l_user_ptr->power_  = l_user["role"].get<power_enum>();
      l_user_ptr->uuid_id_ = l_user_id;
      if (auto l_e = co_await g_ctx().get<sqlite_database>().install(l_user_ptr); !l_e) {
        co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_e.error());
      }
    }
  } catch (...) {
    in_handle->logger_->error("api/auth/authenticated {}", boost::current_exception_diagnostic_information());
  }
  co_return std::move(l_res);
}
}  // namespace
void user_reg(http_route& in_http_route) {
  in_http_route.reg(
      std::make_shared<http_function>(boost::beast::http::verb::get, "api/auth/authenticated", user_authenticated)
  );
}
}  // namespace doodle::http::kitsu