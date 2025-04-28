//
// Created by TD on 25-3-6.
//

#include "http_jwt_fun.h"

#include <doodle_core/metadata/person.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <jwt-cpp/jwt.h>
namespace doodle::http {

std::shared_ptr<http_jwt_fun::http_jwt_t> http_jwt_fun::get_person(const session_data_ptr& in_data) {
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

  return std::make_shared<http_jwt_t>(g_ctx().get<sqlite_database>().get_by_uuid<person>(l_uuid));
}

void http_jwt_fun::http_jwt_t::is_project_manager(const uuid& in_project_id) const {
  if (person_.uuid_id_.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  if (!(                                                                                 //
          person_.role_ == person_role_type::admin ||                                    // 是管理员
          (person_.role_ == person_role_type::manager &&                                 //
           g_ctx().get<sqlite_database>().is_person_in_project(person_, in_project_id))  // 是项目经理并且在项目中
      )                                                                                  //
  )
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}
void http_jwt_fun::http_jwt_t::is_admin() const {
  if (!person_.uuid_id_.is_nil() && person_.role_ == person_role_type::admin) return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

void http_jwt_fun::http_jwt_t::is_manager() const {
  if (!person_.uuid_id_.is_nil() &&
      (person_.role_ == person_role_type::manager || person_.role_ == person_role_type::admin))
    return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

}  // namespace doodle::http