//
// Created by TD on 25-4-8.
//
#include "doodle_core/metadata/person.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/department.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> departments_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  if (person_->role_ != person_role_type::admin)
    throw_exception(http_request_error{boost::beast::http::status::unauthorized, "权限不足"});
  auto l_list = g_ctx().get<sqlite_database>().get_all<department>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}

}  // namespace doodle::http