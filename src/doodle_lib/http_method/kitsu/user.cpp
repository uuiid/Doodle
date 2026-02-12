//
// Created by TD on 24-8-21.
//

#include "doodle_core/core/bcrypt/bcrypt.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/department.h"
#include "doodle_core/metadata/person.h"
#include "doodle_core/metadata/project_status.h"
#include "doodle_core/metadata/status_automation.h"
#include "doodle_core/metadata/task_status.h"
#include "doodle_core/metadata/task_type.h"
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "core/http/http_function.h"
#include "kitsu_reg_url.h"
#include <string>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> user_context::get(session_data_ptr in_handle) {
  nlohmann::json l_ret{};
  auto& l_sql                       = g_ctx().get<sqlite_database>();
  l_ret["asset_types"]              = l_sql.get_asset_types_not_temporal_type();
  l_ret["custom_actions"]           = nlohmann::json::value_t::array;
  l_ret["departments"]              = l_sql.get_all<department>();
  l_ret["notification_count"]       = l_sql.get_notification_count(person_.person_.uuid_id_);
  l_ret["persons"]                  = l_sql.get_all<person>();
  l_ret["project_status"]           = l_sql.get_all<project_status>();
  l_ret["preview_background_files"] = nlohmann::json::value_t::array;
  l_ret["projects"]                 = l_sql.get_project_for_user(person_.person_);
  l_ret["status_automations"]       = l_sql.get_all<status_automation>();
  l_ret["studios"]                  = l_sql.get_all<studio>();
  l_ret["task_status"]              = l_sql.get_all<task_status>();
  l_ret["task_types"]               = l_sql.get_all<task_type>();
  l_ret["user_limit"]               = 9999;
  l_ret["search_filter_groups"]     = nlohmann::json::value_t::object;
  l_ret["search_filters"]           = nlohmann::json::value_t::object;

  co_return in_handle->make_msg(l_ret);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person::get(session_data_ptr in_handle) {
  auto l_p = g_ctx().get<sqlite_database>().get_all<person>();
  co_return in_handle->make_msg(nlohmann::json{} = l_p);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person::post(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_person       = std::make_shared<person>(in_handle->get_json().get<person>());
  l_person->timezone_ = chrono::current_zone()->name();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建用户 email {}", person_.person_.email_,
      person_.person_.get_full_name(), l_person->email_
  );

  co_await g_ctx().get<sqlite_database>().install(l_person);
  auto l_person_deps = std::make_shared<std::vector<person_department_link>>();
  for (auto&& l_dep : l_person->departments_) {
    l_person_deps->emplace_back(person_department_link{.person_id_ = l_person->uuid_id_, .department_id_ = l_dep});
  }
  co_await g_ctx().get<sqlite_database>().install_range(l_person_deps);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建用户 person_id {} email {} 部门数量 {}", person_.person_.email_,
      person_.person_.get_full_name(), l_person->uuid_id_, l_person->email_, l_person->departments_.size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person_instance::put(session_data_ptr in_handle) {
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_person = std::make_shared<person>(l_sql.get_by_uuid<person>(id_));
  in_handle->get_json().get_to(*l_person);
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_persons_change_password, post) {
  person_.check_admin();
  auto l_sql               = g_ctx().get<sqlite_database>();
  auto l_person            = std::make_shared<person>(l_sql.get_by_uuid<person>(person_id_));
  auto l_json              = in_handle->get_json();
  const auto& l_password   = l_json.at("password").get_ref<const std::string&>();
  const auto& l_password_2 = l_json.at("password_2").get_ref<const std::string&>();
  DOODLE_CHICK(l_password == l_password_2, "两次输入密码不一致");
  l_person->password_ = bcrypt::generateHash(l_password);
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(auth_change_password, post) {
  auto l_sql                 = g_ctx().get<sqlite_database>();
  auto l_person              = std::make_shared<person>(l_sql.get_by_uuid<person>(person_.person_.uuid_id_));
  auto l_json                = in_handle->get_json();
  const auto& l_old_password = l_json.at("old_password").get_ref<const std::string&>();
  const auto& l_password     = l_json.at("password").get_ref<const std::string&>();
  const auto& l_password_2   = l_json.at("password_2").get_ref<const std::string&>();
  DOODLE_CHICK(bcrypt::validatePassword(l_old_password, l_person->password_), "旧密码错误");
  DOODLE_CHICK(l_password == l_password_2, "两次输入密码不一致");
  l_person->password_ = bcrypt::generateHash(l_password);
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}
}  // namespace doodle::http
