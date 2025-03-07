//
// Created by TD on 25-3-6.
//

#include "login.h"

#include "doodle_core/metadata/organisation.h"
#include <doodle_core/metadata/person.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> login(session_data_ptr in_handle) {
  co_return in_handle->make_msg("{}");
}

DOODLE_HTTP_FUN(authenticated, get, "api/auth/authenticated", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override {
  nlohmann::json l_r{};
  l_r["authenticated"] = true;
  l_r["user"]          = *person_;
  auto l_org           = g_ctx().get<sqlite_database>().get_all<organisation>();
  l_r["organisation"]  = l_org.empty() ? organisation::get_default() : l_org.front();
  co_return in_handle->make_msg(l_r.dump());
}
DOODLE_HTTP_FUN_END()

}  // namespace
void register_login(http_route& in_r) {
  in_r.reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/auth/login", login))
      .reg(std::make_shared<authenticated_get>());
}
}  // namespace doodle::http