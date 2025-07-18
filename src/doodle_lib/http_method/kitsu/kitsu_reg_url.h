//
// Created by TD on 25-4-1.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
// "api/auth/login"
DOODLE_HTTP_FUN(auth_login, post, ucom_t{} / "api" / "auth" / "login", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/projects"
DOODLE_HTTP_FUN(project_c, post, ucom_t{} / "api" / "data" / "projects", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/projects/{project_id}/settings/task-types"
DOODLE_HTTP_FUN(project_settings_task_types, post, ucom_t{} / "api" / "data" / "projects" / make_cap(g_uuid_regex, &capture_id_t::id_) / "settings" / "task-types", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/data/projects/{project_id}/settings/task-status"
DOODLE_HTTP_FUN(project_settings_task_status, post, ucom_t{} / "api" / "data" / "projects" / make_cap(g_uuid_regex, &capture_id_t::id_) / "settings" / "task-status", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/data/projects/{project_id}/settings/asset-types"
DOODLE_HTTP_FUN(project_settings_asset_types, post, ucom_t{} / "api" / "data" / "projects" / make_cap(g_uuid_regex, &capture_id_t::id_) / "settings" / "asset-types", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/actions/tasks/{task_id}/comment"
DOODLE_HTTP_FUN(task_comment, post, ucom_t{} / "api" / "actions" / "tasks" / make_cap(g_uuid_regex, &capture_id_t::id_) / "comment", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
struct task_comment_add_preview_arg {
  uuid task_id{};
  uuid comment_id{};
};
// "api/actions/tasks/{task_id}/comments/{comment_id}/add-preview"
DOODLE_HTTP_FUN(task_comment_add_preview, post, ucom_t{} / "api" / "actions" / "tasks" / make_cap(g_uuid_regex, &task_comment_add_preview_arg::task_id) / "comments" / make_cap(g_uuid_regex, &task_comment_add_preview_arg::comment_id) / "add-preview" , http_jwt_fun_template<task_comment_add_preview_arg>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<task_comment_add_preview_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/pictures/preview-files/{id}"
DOODLE_HTTP_FUN(pictures_preview_files, post, ucom_t{} / "api" / "pictures" / "preview-files" / make_cap(g_uuid_regex, &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/actions/projects/{project_id}/task-types/{task_type_id}/assets/create-tasks"
struct actions_create_tasks_arg {
  uuid project_id{};
  uuid task_type_id{};
};
DOODLE_HTTP_FUN(actions_create_tasks, post, ucom_t{} / "api" / "actions" / "projects" /
  make_cap(g_uuid_regex, &actions_create_tasks_arg::project_id) / "task-types" /
  make_cap(g_uuid_regex, &actions_create_tasks_arg::task_type_id) / "assets" / "create-tasks" ,
  http_jwt_fun_template<actions_create_tasks_arg>
  )
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<actions_create_tasks_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/data/projects/{project_id}/asset-types/{asset_type_id}/assets/new"
struct projects_assets_new_arg {
  uuid project_id{};
  uuid asset_type_id{};
};
DOODLE_HTTP_FUN(
    projects_assets_new, post,
    ucom_t{} / "api" / "data" / "projects" / &projects_assets_new_arg::project_id  /
        "asset-types" / &projects_assets_new_arg::asset_type_id / "assets" / "new",
    http_jwt_fun_template<projects_assets_new_arg>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<projects_assets_new_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/data/task-status-links"
DOODLE_HTTP_FUN(data_task_status_links, post, ucom_t{} / "api" / "data" / "task-status-links", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "/api/data/persons"
DOODLE_HTTP_FUN(data_person, post, ucom_t{} / "api" / "data" / "persons", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "/api/auth/reset-password"
DOODLE_HTTP_FUN_CONST(auth_reset_password, post, ucom_t{} / "api" / "auth" / "reset-password", http_jwt_fun_template<void>) {
  init();
}
void init();
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// /api/actions/user/notifications/mark-all-as-read
DOODLE_HTTP_FUN(actions_user_notifications_mark_all_as_read, post, ucom_t{} / "api" / "actions" / "user" / "notifications" / "mark-all-as-read", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

// "api/data/comments/{comment_id}"
DOODLE_HTTP_FUN(data_comment, put, ucom_t{} / "api" / "data" / "comments" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/actions/persons/{id}/assign"
DOODLE_HTTP_FUN(actions_persons_assign, put, ucom_t{} / "api" / "actions" / "persons" / &capture_id_t::id_ / "assign", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/data/tasks/{id}"
DOODLE_HTTP_FUN(data_tasks, put, ucom_t{} / "api" / "data" / "tasks" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/data/entities/{id}"
DOODLE_HTTP_FUN(data_entities, put, ucom_t{} / "api" / "data" / "entities" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/actions/preview-files/{id}/set-main-preview"
DOODLE_HTTP_FUN(
    actions_preview_files_set_main_preview, put,
    ucom_t{} / "api" / "actions" / "preview-files" / &capture_id_t::id_ / "set-main-preview", http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "/api/auth/reset-password"
DOODLE_HTTP_FUN(auth_reset_password, put, ucom_t{} / "api" / "auth" / "reset-password", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/persons/{id}"
DOODLE_HTTP_FUN(data_person, put, ucom_t{} / "api" / "data" / "persons" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/data/projects/{id}"
DOODLE_HTTP_FUN(project, put, ucom_t{} / "api" / "data" / "projects" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/actions/tasks/clear-assignation
DOODLE_HTTP_FUN(actions_tasks_clear_assignation, put, ucom_t{} / "api" / "actions" / "tasks" / "clear-assignation", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/user/notifications/{id}
DOODLE_HTTP_FUN(data_user_notification, put, ucom_t{} / "api" / "data" / "user" / "notifications" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()

// "api/data/projects/{project_id}"
DOODLE_HTTP_FUN(project, get, ucom_t{} / "api" / "data" / "projects" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// "api/data/user/context"
DOODLE_HTTP_FUN(user_context, get, ucom_t{} / "api" / "data" / "user" / "context", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/auth/authenticated"
DOODLE_HTTP_FUN(authenticated, get, ucom_t{} / "api" / "auth" / "authenticated", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/organisations"
DOODLE_HTTP_FUN(organisations, get, ucom_t{} / "api" / "data" / "organisations", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/departments"
DOODLE_HTTP_FUN(departments, get, ucom_t{} / "api" / "data" / "departments", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/studios"
DOODLE_HTTP_FUN(studios, get, ucom_t{} / "api" / "data" / "studios", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/task-types"
DOODLE_HTTP_FUN(task_types, get, ucom_t{} / "api" / "data" / "task-types", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/custom-actions"
DOODLE_HTTP_FUN(custom_actions, get, ucom_t{} / "api" / "data" / "custom-actions", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/status-automations"
DOODLE_HTTP_FUN(status_automations, get, ucom_t{} / "api" / "data" / "status-automations", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// "api/data/tasks/{task_id}/comments"
DOODLE_HTTP_FUN(tasks_comments, get, ucom_t{} / "api" / "data" / "tasks" / &capture_id_t::id_ / "comments", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/assets/with-tasks
DOODLE_HTTP_FUN(with_tasks, get, ucom_t{} / "api" / "data" / "assets" / "with-tasks", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/sequences/with-tasks
DOODLE_HTTP_FUN(sequences_with_tasks, get, ucom_t{} / "api" / "data" / "sequences" / "with-tasks", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/assets/{asset_id}
DOODLE_HTTP_FUN(asset_details, get, ucom_t{} / "api" / "data" / "assets" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/projects/{project_id}/assets/shared-used
DOODLE_HTTP_FUN(shared_used, get, ucom_t{} / "api" / "data" / "projects" / &capture_id_t::id_ / "assets" / "shared-used", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/projects/{project_id}/schedule-items/task-types
DOODLE_HTTP_FUN(
    data_project_schedule_items_task_types, get,
    ucom_t{} / "api" / "data" / "projects" / &capture_id_t::id_ / "schedule-items" / "task-types", http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/projects/{project_id}/schedule-items
DOODLE_HTTP_FUN(data_project_schedule_items, get, ucom_t{} / "api" / "data" / "projects" / &capture_id_t::id_ / "schedule-items", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/projects/{project_id}/milestones
DOODLE_HTTP_FUN(data_project_milestones, get, ucom_t{} / "api" / "data" / "projects" / &capture_id_t::id_ / "milestones", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/user/tasks
DOODLE_HTTP_FUN(data_user_tasks, get, ucom_t{} / "api" / "data" / "user" / "tasks", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/user/done-tasks
DOODLE_HTTP_FUN(data_user_done_tasks, get, ucom_t{} / "api" / "data" / "user" / "done-tasks", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/user/tasks-to-check
DOODLE_HTTP_FUN(tasks_to_check, get, ucom_t{} / "api" / "data" / "user" / "tasks-to-check", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/projects/all
DOODLE_HTTP_FUN(project_all, get, ucom_t{} / "api" / "data" / "projects" / "all", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/persons
DOODLE_HTTP_FUN(person_all, get, ucom_t{} / "api" / "data" / "persons", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/user/time-spents
DOODLE_HTTP_FUN(data_user_time_spents_all, get, ucom_t{} / "api" / "data" / "user" / "time-spents", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/user/time-spents/{date}
struct data_user_time_spents_arg {
  chrono::year_month_day date_;
};
DOODLE_HTTP_FUN(data_user_time_spents, get, ucom_t{} / "api" / "data" / "user" / "time-spents" / make_cap(g_year_month_day_regex, &data_user_time_spents_arg::date_), http_jwt_fun_template<data_user_time_spents_arg>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<data_user_time_spents_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
struct person_day_off_arg {
  uuid id_;
  chrono::year_month_day date_;
};
// api/data/persons/{person_id}/day-offs/{date}
DOODLE_HTTP_FUN(person_day_off, get, ucom_t{} / "api" / "data" / "persons" / &person_day_off_arg::id_ / "day-offs" / &person_day_off_arg::date_, http_jwt_fun_template<person_day_off_arg>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<person_day_off_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/persons/{person_id}/day-offs
DOODLE_HTTP_FUN(person_day_off_all, get, ucom_t{} / "api" / "data" / "persons" / &capture_id_t::id_ / "day-offs", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/persons/time-spents/day-table/{year}/{month}
struct year_month_arg {
  std::int32_t year_;
  std::int32_t month_;
};
DOODLE_HTTP_FUN(person_time_spents_day_table, get, ucom_t{} / "api" / "data" / "persons" / "time-spents" / "day-table" / &year_month_arg::year_ / &year_month_arg::month_, http_jwt_fun_template<year_month_arg>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<year_month_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/persons/day-offs/{year}/{month}
DOODLE_HTTP_FUN(person_day_off_1, get, ucom_t{} / "api" / "data" / "persons" / "day-offs" / &year_month_arg::year_ / &year_month_arg::month_, http_jwt_fun_template<year_month_arg>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<year_month_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/config
DOODLE_HTTP_FUN(config, get, ucom_t{} / "api" / "config", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/doodle/deepseek/key
DOODLE_HTTP_FUN(deepseek_key, get, ucom_t{} / "api" / "doodle" / "deepseek" / "key", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// api/data/playlists/entities/{id}/preview-files
DOODLE_HTTP_FUN(
    playlists_entities_preview_files, get,
    ucom_t{} / "api" / "data" / "playlists" / "entities" / &capture_id_t::id_ / "preview-files", http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/organisations/{id}
DOODLE_HTTP_FUN(pictures_thumbnails_organisations, get, ucom_t{} / "api" / "pictures" / "thumbnails" / "organisations" / make_cap(fmt::format("{}.png", g_uuid_regex), &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails-square/preview-files/{id}
DOODLE_HTTP_FUN(
    pictures_thumbnails_square_preview_files, get, ucom_t{} / "api" / "pictures" / "thumbnails-square" / "preview-files" / make_cap(fmt::format("{}.png", g_uuid_regex), &capture_id_t::id_), http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/preview-files/{id}
DOODLE_HTTP_FUN(pictures_thumbnails_preview_files, get, ucom_t{} / "api" / "pictures" / "thumbnails" / "preview-files" / make_cap(fmt::format("{}.png", g_uuid_regex), &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/persons/{id}
DOODLE_HTTP_FUN(pictures_thumbnails_persons, get, ucom_t{} / "api" / "pictures" / "thumbnails" / "persons" / make_cap(fmt::format("{}.png", g_uuid_regex), &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/pictures/originals/preview-files/{id}/download
DOODLE_HTTP_FUN(
    pictures_originals_preview_files_download, get,
    ucom_t{} / "api" / "pictures" / "originals" / "preview-files" / &capture_id_t::id_ / "download", http_jwt_fun_template<capture_id_t>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/pictures/originals/preview-files/{id}
DOODLE_HTTP_FUN(pictures_originals_preview_files, get, ucom_t{} / "api" / "pictures" / "originals" / "preview-files" / make_cap(fmt::format("{}.png", g_uuid_regex), &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()

// /api/pictures/previews/preview-files/{id}
DOODLE_HTTP_FUN(pictures_previews_preview_files, get, ucom_t{} / "api" / "pictures" / "previews" / "preview-files" / make_cap(fmt::format("{}.png", g_uuid_regex), &capture_id_t::id_), http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/data/attachment-files/{id}/file/{file_name}
struct data_attachment_files_file_arg {
  uuid id_;
  FSys::path file_name_;
};
DOODLE_HTTP_FUN(data_attachment_files_file, get, (ucom_t{} / "api" / "data" / "attachment-files" / &data_attachment_files_file_arg::id_ / "file" / &data_attachment_files_file_arg::file_name_), http_jwt_fun_template<data_attachment_files_file_arg>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<data_attachment_files_file_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/data/tasks/open-tasks
DOODLE_HTTP_FUN(data_tasks_open_tasks, get, ucom_t{} / "api" / "data" / "tasks" / "open-tasks", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()
// /api/data/assets/<asset_id>/cast-in
DOODLE_HTTP_FUN(data_assets_cast_in, get, ucom_t{} / "api" / "data" / "assets" / &capture_id_t::id_ / "cast-in", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/data/entities/<entity_id>/news
DOODLE_HTTP_FUN(data_entities_news, get, ucom_t{} / "api" / "data" / "entities" / &capture_id_t::id_ / "news", http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/data/user/notifications
DOODLE_HTTP_FUN(data_user_notifications, get, ucom_t{} / "api" / "data" / "user" / "notifications", http_jwt_fun_template<void>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(session_data_ptr in_handle) override;
DOODLE_HTTP_FUN_END()

// /api/data/tasks/{task_id}/comments/{comment_id}
struct task_comment_arg {
  uuid task_id_;
  uuid comment_id_;
};
DOODLE_HTTP_FUN(task_comment, delete_, ucom_t{} / "api" / "data" / "tasks" / &task_comment_arg::task_id_ / "comments" / &task_comment_arg::comment_id_, http_jwt_fun_template<task_comment_arg>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<task_comment_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/data/assets/{id}
DOODLE_HTTP_FUN(data_asset, delete_, ucom_t{} / "api" / "data" / "assets" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()
// api/data/projects/{project_id}/settings/task-types/{task_type_id}
struct project_settings_task_types_arg {
  uuid project_id_;
  uuid task_type_id_;
};
DOODLE_HTTP_FUN(
    project_settings_task_types, delete_,
    ucom_t{} / "api" / "data" / "projects" / &project_settings_task_types_arg::project_id_ / "settings" / "task-types" /
        &project_settings_task_types_arg::task_type_id_,
    http_jwt_fun_template<project_settings_task_types_arg>
)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<project_settings_task_types_arg> in_arg
) override;
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{id}
DOODLE_HTTP_FUN(data_task, delete_, ucom_t{} / "api" / "data" / "tasks" / &capture_id_t::id_, http_jwt_fun_template<capture_id_t>)
boost::asio::awaitable<boost::beast::http::message_generator> callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) override;
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http