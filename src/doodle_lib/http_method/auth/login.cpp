//
// Created by TD on 25-3-6.
//

#include "login.h"

#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> login(session_data_ptr in_handle) {
  co_return in_handle->make_msg("{}");
}
boost::asio::awaitable<boost::beast::http::message_generator> authenticated(session_data_ptr in_handle) {
  nlohmann::json l_r{};
  l_r["authenticated"] = true;
  l_r["user"]          = {};
  l_r["organisation"]  = {};
  co_return in_handle->make_msg("{}");
}
}  // namespace
void register_login(http_route& in_r) {
  in_r.reg(std::make_shared<http_function>(boost::beast::http::verb::post, "auth/login", login))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "auth/authenticated", authenticated));
}
}  // namespace doodle::http