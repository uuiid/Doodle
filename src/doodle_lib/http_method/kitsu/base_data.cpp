//
// Created by TD on 25-4-8.
//
#include "doodle_core/metadata/person.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> departments::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<department>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> departments_instance::put(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_department_ptr = std::make_shared<department>(l_sql.get_by_uuid<department>(id_));
  in_handle->get_json().get_to(*l_department_ptr);
  co_await l_sql.install(l_department_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_department_ptr);
}

boost::asio::awaitable<boost::beast::http::message_generator> studios::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<studio>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> task_types::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<task_type>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> custom_actions::get(session_data_ptr in_handle) {
  person_.check_admin();

  co_return in_handle->make_msg(nlohmann::json::array());
}
boost::asio::awaitable<boost::beast::http::message_generator> status_automations::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<status_automation>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}

}  // namespace doodle::http