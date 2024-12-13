//
// Created by TD on 24-12-13.
//

#include "epiboly.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> config(session_data_ptr in_handle) {
  co_return in_handle->make_msg(R"(
{
    "is_self_hosted": true,
    "crisp_token": "",
    "dark_theme_by_default": null,
    "indexer_configured": true,
    "saml_enabled": false,
    "saml_idp_name": "",
    "default_locale": "en_US",
    "default_timezone": "Europe/Paris"
}
)"s);
}
boost::asio::awaitable<boost::beast::http::message_generator> authenticated(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::unauthorized, R"({
    "msg": "Missing JWT in cookies or headers Missing cookie \"access_token_cookie\"; Missing Authorization Header"
})"s);
}

boost::asio::awaitable<boost::beast::http::message_generator> user_context(session_data_ptr in_handle) {
  nlohmann::json l_json{};
  try {
    auto& l_database   = g_ctx().get<sqlite_database>();
    auto l_all_prj     = l_database.get_all<project_helper::database_t>();
    l_json["projects"] = l_all_prj;
  } catch (...) {
    in_handle->logger_->error("api/data/user/context {}", boost::current_exception_diagnostic_information());
  }
  co_return in_handle->make_msg(l_json.dump());
}
}  // namespace
void epiboly_reg(http_route& in_http_route) {
  in_http_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/config", config))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::put, "api/auth/authenticated", authenticated))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/user/context", user_context));
}
}  // namespace doodle::http::kitsu