//
// Created by TD on 25-4-8.
//
#include "doodle_core/metadata/entity_type.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include <memory>
#include <vector>


namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> departments::get(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_list = g_ctx().get<sqlite_database>().get_all<department>();
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> departments_instance::put(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_sql            = g_ctx().get<sqlite_database>();
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

boost::asio::awaitable<boost::beast::http::message_generator> data_entity_types_instance::put(
    session_data_ptr in_handle
) {
  person_.check_admin();
  auto l_sql            = g_ctx().get<sqlite_database>();
  auto l_asset_type_ptr = std::make_shared<asset_type>(l_sql.get_by_uuid<asset_type>(id_));
  in_handle->get_json().get_to(*l_asset_type_ptr);

  auto l_task_type_asset_type_link_list = std::make_shared<std::vector<task_type_asset_type_link>>();
  for (auto&& l_task_type_id : l_asset_type_ptr->task_types_) {
    l_task_type_asset_type_link_list->emplace_back(
        task_type_asset_type_link{
            .asset_type_id_ = l_asset_type_ptr->uuid_id_,
            .task_type_id_  = l_task_type_id,
        }
    );
  }
  co_await l_sql.remove_task_type_asset_type_link_by_asset_type(l_asset_type_ptr->uuid_id_);
  co_await l_sql.install_range(l_task_type_asset_type_link_list);
  co_await l_sql.install(l_asset_type_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_asset_type_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_task_status::post(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_sql         = g_ctx().get<sqlite_database>();
  auto l_status      = std::make_shared<task_status>();
  l_status->uuid_id_ = core_set::get_set().get_uuid();
  in_handle->get_json().get_to(*l_status);
  co_await l_sql.install(l_status);
  co_return in_handle->make_msg(nlohmann::json{} = *l_status);
}
}  // namespace doodle::http