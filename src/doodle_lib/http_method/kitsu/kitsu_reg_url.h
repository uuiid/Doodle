//
// Created by TD on 25-4-1.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
// clang-format off
DOODLE_HTTP_FUN(auth_login, post, "api/auth/login", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(project_c, post, "api/data/projects", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(project_settings_task_types, post, "api/data/projects/{project_id}/settings/task-types", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(project_settings_task_status, post, "api/data/projects/{project_id}/settings/task-status", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(project_settings_asset_types, post, "api/data/projects/{project_id}/settings/asset-types", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(actions_create_tasks, post, "api/actions/projects/{project_id}/task-types/{task_type_id}/assets/create-tasks", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(project, get, "api/data/projects/{project_id}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(user_context, get, "api/data/user/context", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(authenticated, get, "api/auth/authenticated", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(organisations, get, "api/data/organisations", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(departments, get, "api/data/departments", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(studios, get, "api/data/studios", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(task_types, get, "api/data/task-types", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(custom_actions, get, "api/data/custom-actions", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(status_automations, get, "api/data/status-automations", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(tasks_comments, get, "api/data/tasks/{task_id}/comments", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(with_tasks, get, "api/data/assets/with-tasks", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(shared_used, get, "api/data/projects/{project_id}/assets/shared-used", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(data_user_tasks, get, "api/data/user/tasks", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(data_user_done_tasks, get, "api/data/user/done-tasks", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(tasks_to_check, get, "api/data/user/tasks-to-check", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(project_all, get, "api/data/projects/all", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(person_all, get, "api/data/persons", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(data_user_time_spents_all, get, "api/data/user/time-spents", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(data_user_time_spents, get, "api/data/user/time-spents/{date}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(person_day_off, get, "api/data/persons/{person_id}/day-offs/{date}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(person_day_off_all, get, "api/data/persons/{person_id}/day-offs", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(person_time_spents_day_table, get, "api/data/persons/time-spents/day-table/{year}/{month}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
DOODLE_HTTP_FUN(person_day_off_1, get, "api/data/persons/day-offs/{year}/{month}", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(config, get, "api/config", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

DOODLE_HTTP_FUN(deepseek_key, get, "api/doodle/deepseek/key", http_jwt_fun)
boost::asio::awaitable<boost::beast::http::message_generator> callback(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// clang-format on
}  // namespace doodle::http