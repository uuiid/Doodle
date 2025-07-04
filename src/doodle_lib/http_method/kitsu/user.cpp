//
// Created by TD on 24-8-21.
//

#include "doodle_core/metadata/department.h"
#include "doodle_core/metadata/project_status.h"
#include "doodle_core/metadata/status_automation.h"
#include "doodle_core/metadata/task_status.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> user_context_get::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  nlohmann::json l_ret{};
  auto& l_sql             = g_ctx().get<sqlite_database>();
  l_ret["asset_types"]    = l_sql.get_asset_types_not_temporal_type();
  l_ret["custom_actions"] = nlohmann::json::value_t::array;
  l_ret["departments"]    = l_sql.get_all<department>();
  for (auto& l_v : g_ctx().get<dingding::dingding_company>().company_info_map_ | std::views::values) {
    l_ret["dingding_companys"].emplace_back(l_v);
  }
  l_ret["notification_count"]       = l_sql.get_notification_count(l_ptr->person_.uuid_id_);
  l_ret["persons"]                  = l_sql.get_all<person>();
  l_ret["project_status"]           = l_sql.get_all<project_status>();
  l_ret["preview_background_files"] = nlohmann::json::value_t::array;
  l_ret["projects"]                 = l_sql.get_project_for_user(l_ptr->person_);
  l_ret["status_automations"]       = l_sql.get_all<status_automation>();
  l_ret["studios"]                  = l_sql.get_all<studio>();
  l_ret["task_status"]              = l_sql.get_all<task_status>();
  l_ret["task_types"]               = l_sql.get_all<task_type>();
  l_ret["user_limit"]               = 9999;
  l_ret["search_filter_groups"]     = nlohmann::json::value_t::object;
  l_ret["search_filters"]           = nlohmann::json::value_t::object;

  co_return in_handle->make_msg(l_ret.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> person_all_get::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  auto l_p   = g_ctx().get<sqlite_database>().get_all<person>();
  // for (auto&& [l_k, l_v, l_has] : in_handle->url_.params()) {
  //   if (l_k == "relations")
  //     for (auto&& i : l_p) i.write_departments_ = true;
  // }
  co_return in_handle->make_msg((nlohmann::json{} = l_p).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person_post::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  l_ptr->is_admin();
  auto l_person       = std::make_shared<person>(in_handle->get_json().get<person>());
  l_person->timezone_ = chrono::current_zone()->name();
  co_await g_ctx().get<sqlite_database>().install(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person_put::callback(session_data_ptr in_handle) {
  auto l_ptr    = get_person(in_handle);
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_person = std::make_shared<person>(l_sql.get_by_uuid<person>(in_handle->capture_->get_uuid()));
  in_handle->get_json().get_to(*l_person);
  co_await l_sql.install(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}

}  // namespace doodle::http
