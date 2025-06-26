//
// Created by TD on 24-8-20.
//

#include "doodle_core/metadata/notification.h"
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_task_status_links_post::callback(
    session_data_ptr in_handle
) {
  auto l_person           = get_person(in_handle);
  auto l_sql              = g_ctx().get<sqlite_database>();
  auto l_json             = in_handle->get_json();
  auto l_task_status_link = std::make_shared<project_task_status_link>(
      l_sql.get_project_task_status_link(l_json["project_id"].get<uuid>(), l_json["task_status_id"].get<uuid>())
          .value_or(project_task_status_link{})
  );
  if (l_task_status_link->uuid_id_.is_nil()) l_task_status_link->uuid_id_ = core_set::get_set().get_uuid();
  l_json.get_to(*l_task_status_link);
  co_await l_sql.install(l_task_status_link);
  co_return in_handle->make_msg(nlohmann::json{} = *l_task_status_link);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_tasks_put::callback(session_data_ptr in_handle) {
  auto l_person = get_person(in_handle);
  auto l_sql    = g_ctx().get<sqlite_database>();
  auto l_task   = std::make_shared<task>(l_sql.get_by_uuid<task>(in_handle->capture_->get_uuid()));
  l_person->check_task_action_access(*l_task);
  in_handle->get_json().get_to(*l_task);
  co_await l_sql.install(l_task);
  // l_task->assigner_id_ = l_person->person_.uuid_id_;
  co_return in_handle->make_msg(nlohmann::json{} = *l_task);
}

boost::asio::awaitable<boost::beast::http::message_generator> actions_persons_assign_put::callback(
    session_data_ptr in_handle
) {
  auto l_person      = get_person(in_handle);
  auto l_sql         = g_ctx().get<sqlite_database>();
  auto l_person_data = l_sql.get_by_uuid<person>(in_handle->capture_->get_uuid());
  auto l_task_ids    = in_handle->get_json()["task_ids"].get<std::vector<uuid>>();
  nlohmann::json l_ret{};
  for (auto&& l_task_id : l_task_ids) {
    auto l_task          = std::make_shared<task>(l_sql.get_by_uuid<task>(l_task_id));
    auto l_task_assign   = std::make_shared<assignees_table>();
    l_task->assigner_id_ = l_person->person_.uuid_id_;
    co_await l_sql.install(l_task);
    l_task_assign->person_id_ = l_person_data.uuid_id_;
    l_task_assign->task_id_   = l_task->uuid_id_;
    co_await l_sql.install(l_task_assign);
    // 这里需要检查一下, 任务的分配人是否是当前用户
    if (l_person->person_.uuid_id_ != l_person_data.uuid_id_) {
      auto l_notification        = std::make_shared<notification>();
      l_notification->type_      = notification_type::assignation;
      l_notification->task_id_   = l_task->uuid_id_;
      l_notification->author_id_ = l_person->person_.uuid_id_;
      l_notification->person_id_ = l_person_data.uuid_id_;
      co_await l_sql.install(l_notification);
    }
    l_ret.emplace_back(*l_task);
  }
  co_return in_handle->make_msg(l_ret);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_tasks_get::callback(
    session_data_ptr in_handle
) {
  auto l_ptr = get_person(in_handle);
  auto& sql  = g_ctx().get<sqlite_database>();
  auto l_p1  = sql.get_person_tasks(l_ptr->person_);
  co_return in_handle->make_msg((nlohmann::json{} = l_p1).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_done_tasks_get::callback(
    session_data_ptr in_handle
) {
  auto l_ptr = get_person(in_handle);
  auto& sql  = g_ctx().get<sqlite_database>();
  auto l_p1  = sql.get_person_tasks(l_ptr->person_, true);
  co_return in_handle->make_msg((nlohmann::json{} = l_p1).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> tasks_to_check_get::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);

  switch (l_ptr->person_.role_) {
    case person_role_type::admin:
    case person_role_type::supervisor:
    case person_role_type::manager:
      break;

    case person_role_type::user:
    case person_role_type::client:
    case person_role_type::vendor:
      co_return in_handle->make_msg("[]"s);
      break;
  }

  auto& sql = g_ctx().get<sqlite_database>();
  auto l_p1 = sql.get_preson_tasks_to_check(l_ptr->person_);
  co_return in_handle->make_msg((nlohmann::json{} = l_p1).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> tasks_comments_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  auto l_task_id = from_uuid_str(in_handle->capture_->get("task_id"));

  auto& sql      = g_ctx().get<sqlite_database>();
  nlohmann::json l_r{};
  l_r = sql.get_comments(l_task_id);
  co_return in_handle->make_msg(l_r);
}


}  // namespace doodle::http
