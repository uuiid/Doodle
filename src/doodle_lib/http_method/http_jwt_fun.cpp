//
// Created by TD on 25-3-6.
//

#include "http_jwt_fun.h"

#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/http_method/kitsu/kitsu.h>

#include <jwt-cpp/jwt.h>
namespace doodle::http {

void http_jwt_fun::parse_header(const session_data_ptr& in_handle) {
  auto l_jwt = in_handle->req_header_[boost::beast::http::field::cookie];
  if (l_jwt.empty()) l_jwt = in_handle->req_header_[boost::beast::http::field::authorization];

  if (auto l_it = l_jwt.find("access_token_cookie="); l_it != std::string::npos)
    l_jwt = l_jwt.substr(l_it + 20, l_jwt.find(';', l_it) - l_it - 20);
  else if (auto l_it_b = l_jwt.find("Bearer "); l_it_b != std::string::npos)
    l_jwt = l_jwt.substr(l_it_b + 7, l_jwt.find(' ', l_it_b) - l_it_b - 7);
  if (l_jwt.empty()) throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先登录"});
  // std::string l_l_jwt_str{l_jwt};
  if (!g_ctx().contains<kitsu_ctx_t>()) return;

  auto& l_ctx       = g_ctx().get<kitsu_ctx_t>();

  auto verifier     = jwt::verify().allow_algorithm(jwt::algorithm::hs256{l_ctx.secret_});
  auto l_jwt_decode = jwt::decode(l_jwt);
  try {
    verifier.verify(l_jwt_decode);
  } catch (const std::system_error& e) {
    throw_exception(
        http_request_error{boost::beast::http::status::unauthorized, fmt::format("cookie 验证错误 {}", e.what())}
    );
  }
  auto l_uuid = from_uuid_str(l_jwt_decode.get_subject());
  auto& l_sql = g_ctx().get<sqlite_database>();
  // default_logger_raw()->warn("{}", l_uuid);
  if (l_sql.uuid_to_id<person>(l_uuid) == 0)
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先注册"});
  person_ = {l_sql.get_by_uuid<person>(l_uuid)};
}

std::shared_ptr<http_jwt_fun::http_jwt_t> http_jwt_fun::get_person(const session_data_ptr& in_data) {
  auto l_jwt = in_data->req_header_[boost::beast::http::field::cookie];
  if (l_jwt.empty()) l_jwt = in_data->req_header_[boost::beast::http::field::authorization];

  if (auto l_it = l_jwt.find("access_token_cookie="); l_it != std::string::npos)
    l_jwt = l_jwt.substr(l_it + 20, l_jwt.find(';', l_it) - l_it - 20);
  else if (auto l_it_b = l_jwt.find("Bearer "); l_it_b != std::string::npos)
    l_jwt = l_jwt.substr(l_it_b + 7, l_jwt.find(' ', l_it_b) - l_it_b - 7);
  if (l_jwt.empty()) throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先登录"});
  // std::string l_l_jwt_str{l_jwt};
  auto& l_ctx       = g_ctx().get<kitsu_ctx_t>();

  auto verifier     = jwt::verify().allow_algorithm(jwt::algorithm::hs256{l_ctx.secret_});
  auto l_jwt_decode = jwt::decode(l_jwt);
  try {
    verifier.verify(l_jwt_decode);
  } catch (const std::system_error& e) {
    throw_exception(
        http_request_error{boost::beast::http::status::unauthorized, fmt::format("cookie 验证错误 {}", e.what())}
    );
  }
  auto l_uuid = from_uuid_str(l_jwt_decode.get_subject());
  auto& l_sql = g_ctx().get<sqlite_database>();
  // default_logger_raw()->warn("{}", l_uuid);
  if (l_sql.uuid_to_id<person>(l_uuid) == 0)
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "请先注册"});

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
void http_jwt_fun::http_jwt_t::is_project_access(const uuid& in_project_id) const {
  if (person_.uuid_id_.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  if (!(                                                                               //
          person_.role_ == person_role_type::admin ||                                  // 是管理员
          person_.role_ == person_role_type::manager ||                                // 是项目经理
          g_ctx().get<sqlite_database>().is_person_in_project(person_, in_project_id)  // 在项目中
      )                                                                                //
  )
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}
void http_jwt_fun::http_jwt_t::check_task_assign_access(const uuid& in_project_id) const {
  if (person_.uuid_id_.is_nil())
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  if (!(                                                                                 //
          (person_.role_ == person_role_type::admin ||                                   // 是管理员
           person_.role_ == person_role_type::manager ||                                 // 是项目经理
           g_ctx().get<sqlite_database>().is_person_in_project(person_, in_project_id))  // 在项目中
          && person_.role_ != person_role_type::user
      )  //
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
void http_jwt_fun::http_jwt_t::check_task_action_access(const uuid& in_task_id) const {
  check_task_action_access(g_ctx().get<sqlite_database>().get_by_uuid<task>(in_task_id));
}
void http_jwt_fun::http_jwt_t::check_task_action_access(const task& in_task_id) const {
  if (person_.role_ == person_role_type::admin) return;
  auto l_sql = g_ctx().get<sqlite_database>();
  if (!l_sql.is_person_in_project(person_, in_task_id.project_id_))
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  if (person_.role_ == person_role_type::manager || person_.role_ == person_role_type::supervisor) return;

  if (l_sql.is_task_assigned_to_person(in_task_id.uuid_id_, person_.uuid_id_)) return;
  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

void http_jwt_fun::http_jwt_t::check_task_department_access(const task& in_task_id, const person& in_person_id) const {
  if (person_.role_ == person_role_type::admin || person_.role_ == person_role_type::manager) return;
  auto l_sql = g_ctx().get<sqlite_database>();
  if (l_sql.is_person_in_project(person_, in_task_id.project_id_)) return;

  auto l_task_type = l_sql.get_by_uuid<task_type>(in_task_id.task_type_id_);
  if (person_.role_ == person_role_type::supervisor) {
    if (person_.departments_.empty()) return;
    if (std::ranges::find(person_.departments_, l_task_type.uuid_id_) != person_.departments_.end() &&
        !in_person_id.departments_.empty())
      return;
  }
  if (std::ranges::find(in_person_id.departments_, l_task_type.uuid_id_) != in_person_id.departments_.end() &&
      person_.uuid_id_ == in_person_id.uuid_id_)
    return;

  throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}
void http_jwt_fun::http_jwt_t::check_delete_access(const uuid& in_project_id) const {
  if (person_.role_ == person_role_type::admin || person_.role_ == person_role_type::manager) return;
  auto l_sql = g_ctx().get<sqlite_database>();
  if (!l_sql.is_person_in_project(person_, in_project_id))
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
}

}  // namespace doodle::http