//
// Created by TD on 25-3-6.
//

#include "http_jwt_fun.h"

#include <doodle_core/metadata/person.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <jwt-cpp/jwt.h>
namespace doodle::http {

void http_jwt_fun::get_person(const session_data_ptr& in_data) {
  auto l_jwt = in_data->req_header_[boost::beast::http::field::cookie];
  if (l_jwt.empty()) l_jwt = in_data->req_header_[boost::beast::http::field::authorization];

  if (auto l_it = l_jwt.find("access_token_cookie="); l_it != std::string::npos)
    l_jwt = l_jwt.substr(l_it + 20, l_jwt.find(';', l_it) - l_it - 20);
  else if (auto l_it_b = l_jwt.find("Bearer "); l_it_b != std::string::npos)
    l_jwt = l_jwt.substr(l_it_b + 7, l_jwt.find(' ', l_it_b) - l_it_b - 7);
  if (l_jwt.empty()) throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先登录"});
  // std::string l_l_jwt_str{l_jwt};
  auto l_uuid = from_uuid_str(jwt::decode(l_jwt).get_payload_claim("sub").as_string());
  auto& l_sql = g_ctx().get<sqlite_database>();
  // default_logger_raw()->warn("{}", l_uuid);
  if (l_sql.uuid_to_id<person>(l_uuid) == 0)
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先登录"});
  person_ = std::make_shared<person>(g_ctx().get<sqlite_database>().get_by_uuid<person>(l_uuid));
}
boost::asio::awaitable<boost::beast::http::message_generator> http_jwt_fun::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  return http_function::callback(in_handle);
}
}  // namespace doodle::http