//
// Created by TD on 25-4-1.
//

#pragma once
#include "doodle_core/doodle_core_fwd.h"

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/http_method/http_jwt_fun.h>

#include <core/http/http_function.h>
#include <string>

namespace doodle::http {
// /api/data/updata-logs
DOODLE_HTTP_JWT_FUN(data_updata_logs)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/data/production-specifications
DOODLE_HTTP_JWT_FUN(data_production_specifications)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/auth/login
DOODLE_HTTP_FUN(auth_login)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/auth/login")
DOODLE_HTTP_FUN_END()
// /api/auth/refresh-token
DOODLE_HTTP_JWT_FUN(auth_refresh_token)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/auth/refresh-token")
DOODLE_HTTP_FUN_END()
// /api/data/projects
DOODLE_HTTP_JWT_FUN(data_projects)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects")
DOODLE_HTTP_FUN_END()
// /api/data/preview-files/{preview_file_id}
DOODLE_HTTP_JWT_FUN(data_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{}/settings/task-types
DOODLE_HTTP_JWT_FUN(data_project_settings_task_types)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/settings/task-types", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/settings/task-status
DOODLE_HTTP_JWT_FUN(data_project_settings_task_status)
DOODLE_HTTP_FUN_OVERRIDE(post);
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/settings/task-status", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/settings/asset-types
DOODLE_HTTP_JWT_FUN(data_project_settings_asset_types)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/settings/asset-types", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/comment
DOODLE_HTTP_JWT_FUN(actions_tasks_comment)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{id_}/comment", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/modify-date-comment
DOODLE_HTTP_JWT_FUN(actions_tasks_modify_date_comment)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{id_}/modify-date-comment", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/entity/{entity_id}/working-file
DOODLE_HTTP_JWT_FUN(actions_entity_working_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/entity/{id_}/working-file", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/entity/working-file-many
DOODLE_HTTP_JWT_FUN(actions_projects_entity_working_file_many)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{id_}/entity/working-file-many", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/tasks/{task_id}/export-rig-sk
DOODLE_HTTP_JWT_FUN(actions_tasks_export_rig_sk)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/export-rig-sk", task_id_)
uuid task_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/export-anim-fbx
DOODLE_HTTP_JWT_FUN(actions_tasks_export_anim_fbx)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/export-anim-fbx", task_id_)
uuid task_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/sync/export-anim-fbx
DOODLE_HTTP_JWT_FUN(actions_tasks_sync_export_anim_fbx)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/sync/export-anim-fbx", task_id_)
uuid task_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/sync
DOODLE_HTTP_JWT_FUN(actions_tasks_sync)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/sync", task_id_)
uuid task_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/assets/update/ue
DOODLE_HTTP_JWT_FUN(actions_tasks_assets_update_ue)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/assets/update/ue", task_id_)
uuid task_id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/projects/{project_id}/shots/{shot_id}/working-file
DOODLE_HTTP_JWT_FUN(actions_projects_shots_working_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/shots/{id_}/working-file", project_id_, id_)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/shots/{shot_id}/run-ue-assembly
DOODLE_HTTP_JWT_FUN(actions_projects_shots_run_ue_assembly)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/shots/{id_}/run-ue-assembly", project_id_, id_)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/shots/{shot_id}/run-export-anim-fbx
DOODLE_HTTP_JWT_FUN(actions_projects_shots_run_export_anim_fbx)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/shots/{id_}/run-export-anim-fbx", project_id_, id_)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/sequences/{sequence_id}/casting/ue-assembly-harvest
DOODLE_HTTP_JWT_FUN(actions_projects_sequences_casting_ue_assembly_harvest)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/actions/projects/{project_id_}/sequences/{id_}/casting/ue-assembly-harvest", project_id_, id_
)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/projects/{project_id}/sequences/{sequence_id}/working-file
DOODLE_HTTP_JWT_FUN(actions_projects_sequences_working_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/sequences/{id_}/working-file", project_id_, id_)
uuid project_id_{};
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/data/projects/{project_id}/shots
DOODLE_HTTP_JWT_FUN(data_project_shots)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects/{project_id_}/shots", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/assets
DOODLE_HTTP_JWT_FUN(data_assets)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/assets")
DOODLE_HTTP_FUN_END()
// /api/data/output-types
DOODLE_HTTP_JWT_FUN(data_output_types)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/output-types")
DOODLE_HTTP_FUN_END()
// /api/data/file-status
DOODLE_HTTP_JWT_FUN(data_file_status)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/file-status")
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/comments/{comment_id}/add-preview
DOODLE_HTTP_JWT_FUN(actions_tasks_comments_add_preview)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/comments/{comment_id_}/add-preview", task_id_, comment_id_)
uuid task_id_{};
uuid comment_id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/pictures/preview-files/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/entities/{entity_id}/preview-files
DOODLE_HTTP_JWT_FUN(data_entities_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/entities/{entity_id_}/preview-files", entity_id_)
uuid entity_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/task-types/{task_type_id}/assets/create-tasks
DOODLE_HTTP_JWT_FUN(actions_create_tasks)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/actions/projects/{project_id_}/task-types/{task_type_id_}/assets/create-tasks", project_id_, task_type_id_
)
uuid project_id_{};
uuid task_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/asset-types/{asset_type_id}/assets/new
DOODLE_HTTP_JWT_FUN(projects_assets_new)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/asset-types/{asset_type_id_}/assets/new", project_id_, asset_type_id_
)
uuid project_id_{};
uuid asset_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/task-status-links
DOODLE_HTTP_JWT_FUN(data_task_status_links)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/task-status-links")
DOODLE_HTTP_FUN_END()
// /api/data/persons
DOODLE_HTTP_JWT_FUN(data_person)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/persons")
DOODLE_HTTP_FUN_END()
// /api/auth/reset-password
DOODLE_HTTP_FUN(auth_reset_password)
void init();
auth_reset_password() { init(); }
class impl;
std::shared_ptr<impl> pimpl_{nullptr};
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/auth/reset-password")
DOODLE_HTTP_FUN_END()
// /api/actions/user/notifications/mark-all-as-read
DOODLE_HTTP_JWT_FUN(actions_user_notifications_mark_all_as_read)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/user/notifications/mark-all-as-read")
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/tasks/comment-many
DOODLE_HTTP_JWT_FUN(actions_projects_tasks_comment_many)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{id_}/tasks/comment-many", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{task_id}/comments/{comment_id}/ack
DOODLE_HTTP_JWT_FUN(data_tasks_comments_ack)
DOODLE_HTTP_FUN_OVERRIDE(post);
DOODLE_HTTP_ROUTE("/api/data/tasks/{task_id_}/comments/{comment_id_}/ack", task_id_, comment_id_)
uuid task_id_{};
uuid comment_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/team
DOODLE_HTTP_JWT_FUN(data_projects_team)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/team", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/comments/{comment_id}/preview-files/{preview_file_id}
DOODLE_HTTP_JWT_FUN(actions_tasks_comments_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/actions/tasks/{task_id_}/comments/{comment_id_}/preview-files/{preview_file_id_}", task_id_, comment_id_,
    preview_file_id_
)
uuid task_id_{};
uuid comment_id_{};
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/comments/{comment_id}
DOODLE_HTTP_JWT_FUN(data_comment)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/comments/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/persons/{id}/assign
DOODLE_HTTP_JWT_FUN(actions_persons_assign)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/actions/persons/{id_}/assign", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{id}
DOODLE_HTTP_JWT_FUN(data_tasks)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/tasks/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/entities/{id}
DOODLE_HTTP_JWT_FUN(data_entities)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/data/entities/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/preview-files/{id}/set-main-preview
DOODLE_HTTP_JWT_FUN(actions_preview_files_set_main_preview)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/actions/preview-files/{id_}/set-main-preview", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/persons/{id}
DOODLE_HTTP_JWT_FUN(data_person_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/data/persons/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{}
DOODLE_HTTP_JWT_FUN(data_project_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/clear-assignation
DOODLE_HTTP_JWT_FUN(actions_tasks_clear_assignation)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/actions/tasks/clear-assignation")
DOODLE_HTTP_FUN_END()
// /api/data/user/notifications/{id}
DOODLE_HTTP_JWT_FUN(data_user_notification)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/data/user/notifications/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/data/user/context
DOODLE_HTTP_JWT_FUN(user_context)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/user/context")
DOODLE_HTTP_FUN_END()
// /api/auth/authenticated
DOODLE_HTTP_JWT_FUN(authenticated)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/auth/authenticated")
DOODLE_HTTP_FUN_END()
// /api/data/organisations
DOODLE_HTTP_JWT_FUN(organisations)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/organisations")
DOODLE_HTTP_FUN_END()
// /api/data/departments
DOODLE_HTTP_JWT_FUN(departments)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/departments")
DOODLE_HTTP_FUN_END()
// /api/data/departments/{id}
DOODLE_HTTP_JWT_FUN(departments_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/data/departments/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/studios
DOODLE_HTTP_JWT_FUN(studios)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/studios")
DOODLE_HTTP_FUN_END()
// /api/data/studios/{id}
DOODLE_HTTP_JWT_FUN(studios_instance)

DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)

DOODLE_HTTP_ROUTE("/api/data/studios/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/task-types
DOODLE_HTTP_JWT_FUN(task_types)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/task-types")
DOODLE_HTTP_FUN_END()
// /api/data/custom-actions
DOODLE_HTTP_JWT_FUN(custom_actions)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/custom-actions")
DOODLE_HTTP_FUN_END()
// /api/data/status-automations
DOODLE_HTTP_JWT_FUN(status_automations)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/status-automations")
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{task_id}/comments
DOODLE_HTTP_JWT_FUN(tasks_comments)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/tasks/{id_}/comments", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/assets/with-tasks
DOODLE_HTTP_JWT_FUN(data_assets_with_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/assets/with-tasks")
DOODLE_HTTP_FUN_END()
// /api/data/sequences/with-tasks
DOODLE_HTTP_JWT_FUN(sequences_with_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/sequences/with-tasks")
DOODLE_HTTP_FUN_END()
// /api/data/shots/with-tasks
DOODLE_HTTP_JWT_FUN(data_shots_with_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/shots/with-tasks")
DOODLE_HTTP_FUN_END()
// /api/data/assets/{asset_id}
DOODLE_HTTP_JWT_FUN(asset_details)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/assets/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/assets/shared-used
DOODLE_HTTP_JWT_FUN(shared_used)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/assets/shared-used", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/schedule-items/task-types
DOODLE_HTTP_JWT_FUN(data_project_schedule_items_task_types)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/schedule-items/task-types", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/schedule-items
DOODLE_HTTP_JWT_FUN(data_project_schedule_items)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/schedule-items", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/milestones
DOODLE_HTTP_JWT_FUN(data_project_milestones)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/milestones", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/user/tasks
DOODLE_HTTP_JWT_FUN(data_user_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/user/tasks")
DOODLE_HTTP_FUN_END()
// /api/data/user/done-tasks
DOODLE_HTTP_JWT_FUN(data_user_done_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/user/done-tasks")
DOODLE_HTTP_FUN_END()
// /api/data/user/tasks-to-check
DOODLE_HTTP_JWT_FUN(tasks_to_check)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/user/tasks-to-check")
DOODLE_HTTP_FUN_END()
// /api/data/projects/all
DOODLE_HTTP_JWT_FUN(project_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/all")
DOODLE_HTTP_FUN_END()
// /api/data/persons
DOODLE_HTTP_JWT_FUN(person_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/persons")
DOODLE_HTTP_FUN_END()
// /api/data/user/time-spents
DOODLE_HTTP_JWT_FUN(data_user_time_spents_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/user/time-spents")
DOODLE_HTTP_FUN_END()
// /api/data/user/time-spents/{date}
DOODLE_HTTP_JWT_FUN(data_user_time_spents)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/user/time-spents/{date_}", date_)
chrono::year_month_day date_;
DOODLE_HTTP_FUN_END()

// /api/data/persons/{person_id}/day-offs/{date}
DOODLE_HTTP_JWT_FUN(person_day_off)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/persons/{id_}/day-offs/{date_}", id_, date_)
uuid id_;
chrono::year_month_day date_;
DOODLE_HTTP_FUN_END()
// /api/data/persons/{person_id}/day-offs
DOODLE_HTTP_JWT_FUN(person_day_off_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/persons/{id_}/day-offs", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/persons/time-spents/day-table/{year}/{month}
DOODLE_HTTP_JWT_FUN(person_time_spents_day_table)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/persons/time-spents/day-table/{year_}/{month_}", year_, month_)
std::int32_t year_;
std::int32_t month_;
DOODLE_HTTP_FUN_END()
// /api/data/persons/day-offs/{year}/{month}
DOODLE_HTTP_JWT_FUN(person_day_off_1)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/persons/day-offs/{year_}/{month_}", year_, month_)
std::int32_t year_;
std::int32_t month_;
DOODLE_HTTP_FUN_END()
// /api/config
DOODLE_HTTP_JWT_FUN(config)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/config")
DOODLE_HTTP_FUN_END()

// /api/data/playlists/entities/{id}/preview-files
DOODLE_HTTP_JWT_FUN(playlists_entities_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/playlists/entities/{id_}/preview-files", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/organisations/{id}.png
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_organisations_png)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/pictures/thumbnails/organisations/{id_}.png", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/organisations/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_organisations)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/pictures/thumbnails/organisations/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails-square/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_square_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/pictures/thumbnails-square/preview-files/{id_}.png", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/pictures/thumbnails/preview-files/{id_}.png", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/persons/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_persons)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/pictures/thumbnails/persons/{id_}.png", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/originals/preview-files/{id}/download
DOODLE_HTTP_JWT_FUN(pictures_originals_preview_files_download)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/pictures/originals/preview-files/{id_}/download", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/originals/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_originals_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/pictures/originals/preview-files/{id_}.png", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/pictures/previews/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_previews_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/pictures/previews/preview-files/{id_}.png", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/attachment-files/{id}/file/{file_name}
DOODLE_HTTP_JWT_FUN(data_attachment_files_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/attachment-files/{id_}/file/{file_name_}", id_, file_name_)
uuid id_;
FSys::path file_name_;
DOODLE_HTTP_FUN_END()
// /api/data/tasks/open-tasks
DOODLE_HTTP_JWT_FUN(data_tasks_open_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/tasks/open-tasks")
DOODLE_HTTP_FUN_END()
// /api/data/assets/<asset_id>/cast-in
DOODLE_HTTP_JWT_FUN(data_assets_cast_in)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/assets/{id_}/cast-in", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/entities/<entity_id>/news
DOODLE_HTTP_JWT_FUN(data_entities_news)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/entities/{id_}/news", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/user/notifications
DOODLE_HTTP_JWT_FUN(data_user_notifications)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/user/notifications")
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{id}/full
DOODLE_HTTP_JWT_FUN(data_tasks_full)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/tasks/{id_}/full", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/auth/logout
DOODLE_HTTP_JWT_FUN(auth_logout)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/auth/logout")
DOODLE_HTTP_FUN_END()
// /api/movies/low/preview-files/{id}.mp4
DOODLE_HTTP_JWT_FUN(movies_low_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/movies/low/preview-files/{id_}.mp4", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/movies/originals/preview-files/{id}.mp4
DOODLE_HTTP_JWT_FUN(movies_originals_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/movies/originals/preview-files/{id_}.mp4", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/movies/tiles/preview-files/{id}.png
DOODLE_HTTP_JWT_FUN(movies_tiles_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/movies/tiles/preview-files/{id_}.png", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/data/tasks/{task_id}/comments/{comment_id}
DOODLE_HTTP_JWT_FUN(task_comment)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/tasks/{task_id_}/comments/{comment_id_}", task_id_, comment_id_)
uuid task_id_;
uuid comment_id_;
DOODLE_HTTP_FUN_END()

// /api/data/projects/{project_id}/settings/task-types/{task_type_id}
DOODLE_HTTP_JWT_FUN(project_settings_task_types)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/settings/task-types/{task_type_id_}", project_id_, task_type_id_
)
uuid project_id_;
uuid task_type_id_;
DOODLE_HTTP_FUN_END()

// /api/data/projects/{project_id}/team/{person_id}
DOODLE_HTTP_JWT_FUN(data_project_team_person)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/projects/{project_id_}/team/{person_id_}", project_id_, person_id_)
uuid project_id_;
uuid person_id_;
DOODLE_HTTP_FUN_END()
// /api/data/projects/{}/sequences
DOODLE_HTTP_JWT_FUN(data_project_sequences)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/sequences", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/preview-files/<preview_file_id>/update-annotations
DOODLE_HTTP_JWT_FUN(actions_preview_files_update_annotations)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/actions/preview-files/{preview_file_id_}/update-annotations", preview_file_id_)
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/<project_id>/playlists/temp
DOODLE_HTTP_JWT_FUN(data_project_playlists_temp)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects/{project_id_}/playlists/temp", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/sequences/{sequence_id}
DOODLE_HTTP_JWT_FUN(data_sequence_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/sequences/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/<project_id>/task-types/<task_type_id>/create-tasks/<entity_type>
DOODLE_HTTP_JWT_FUN(actions_projects_task_types_create_tasks)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/actions/projects/{project_id_}/task-types/{task_type_id_}/create-tasks/{entity_type_}", project_id_,
    task_type_id_, entity_type_
)
enum class url_entity_type_enum { episode, sequence };
uuid project_id_{};
uuid task_type_id_{};
url_entity_type_enum entity_type_{};
DOODLE_HTTP_FUN_END()
// /api/data/task-type-links
DOODLE_HTTP_JWT_FUN(data_task_type_links)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/task-type-links")
DOODLE_HTTP_FUN_END()
// /api/data/shots/{shot_id}
DOODLE_HTTP_JWT_FUN(data_shot)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/shots/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/task-types/{task_type_id}/shots/create-tasks
DOODLE_HTTP_JWT_FUN(actions_projects_task_types_shots_create_tasks)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/actions/projects/{project_id_}/task-types/{task_type_id_}/shots/create-tasks", project_id_, task_type_id_
)
uuid project_id_{};
uuid task_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/shots/import/frame-range
DOODLE_HTTP_JWT_FUN(actions_projects_shots_import_frame_range)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/shots/import/frame-range", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/projects/{project_id}/sequences/all/casting
DOODLE_HTTP_JWT_FUN(data_project_sequences_all_casting)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{project_id_}/sequences/all/casting", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/entities/{entity_id}/casting
DOODLE_HTTP_JWT_FUN(data_project_entities_casting)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/entities/{entity_id_}/casting", project_id_, entity_id_
)
uuid project_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/casting/replace
DOODLE_HTTP_JWT_FUN(actions_projects_casting_replace)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/casting/replace", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/casting/copy
DOODLE_HTTP_JWT_FUN(actions_projects_casting_copy)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/casting/copy", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/sequences/{sequence_id}/casting
DOODLE_HTTP_JWT_FUN(data_project_sequences_casting)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/sequences/{sequence_id_}/casting", project_id_, sequence_id_
)
uuid project_id_{};
uuid sequence_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/asset-types/{asset_type_id}/casting
DOODLE_HTTP_JWT_FUN(data_project_asset_types_casting)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/asset-types/{asset_type_id_}/casting", project_id_, asset_type_id_
)
uuid project_id_{};
uuid asset_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/playlists?sort_by=updated_at&page=1
DOODLE_HTTP_JWT_FUN(data_project_playlists)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/projects/{project_id_}/playlists", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/playlists
DOODLE_HTTP_JWT_FUN(data_playlists)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/playlists")
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/playlists/{playlist_id}
DOODLE_HTTP_JWT_FUN(data_project_playlists_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/playlists/{playlist_id_}", project_id_, playlist_id_
)
uuid project_id_{};
uuid playlist_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/playlists/{playlist_id}
DOODLE_HTTP_JWT_FUN(data_playlists_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/playlists/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/playlists/{playlist_id}/entities/{entity_id}
DOODLE_HTTP_JWT_FUN(data_playlists_instance_entity_instance)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/data/playlists/{playlist_id_}/entities/{entity_id_}", playlist_id_, entity_id_
)
uuid playlist_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/playlists/{playlist_id}/shots/{shots_id}
DOODLE_HTTP_JWT_FUN(data_playlists_instance_shots)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/playlists/{playlist_id_}/shots/{shot_id_}", playlist_id_, shot_id_)
uuid playlist_id_{};
uuid shot_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/sequences/{sequence_id}/create-review/playlists
DOODLE_HTTP_JWT_FUN(actions_sequences_create_review_playlists)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/sequences/{sequence_id_}/create-review/playlists", sequence_id_)
uuid sequence_id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/playlists/{playlist_id}/preview-files/{preview_file_id}/create-review
DOODLE_HTTP_JWT_FUN(actions_playlists_preview_files_create_review)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE(
    "/api/actions/playlists/{playlist_id_}/preview-files/{preview_file_id_}/create-review", playlist_id_,
    preview_file_id_
)
uuid playlist_id_{};
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/entity-types/{id}
DOODLE_HTTP_JWT_FUN(data_entity_types_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/entity-types/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/entity-types
DOODLE_HTTP_JWT_FUN(data_entity_types)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/entity-types")
DOODLE_HTTP_FUN_END()
// /api/data/projects/{id}/settings/status-automations
DOODLE_HTTP_JWT_FUN(data_project_settings_status_automations)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects/{id_}/settings/status-automations", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/task-status
DOODLE_HTTP_JWT_FUN(data_task_status)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/task-status")
DOODLE_HTTP_FUN_END()
// /api/data/task-status/{id}
DOODLE_HTTP_JWT_FUN(data_task_status_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/data/task-status/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/doodle/backup
DOODLE_HTTP_JWT_FUN(doodle_backup)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/backup")
DOODLE_HTTP_FUN_END()
// /api/doodle/stop-server
DOODLE_HTTP_JWT_FUN(doodle_stop_server)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/stop-server")
DOODLE_HTTP_FUN_END()

// /api/actions/preview-files/{preview_file_id}/compose-video
DOODLE_HTTP_JWT_FUN(actions_preview_files_compose_video)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/preview-files/{preview_file_id_}/compose-video", preview_file_id_)
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/tasks/{task_id}/create-review
DOODLE_HTTP_JWT_FUN(actions_tasks_create_review)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/tasks/{task_id_}/create-review", task_id_)
uuid task_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/settings/status-automations/{status_automation_id}
DOODLE_HTTP_JWT_FUN(data_project_settings_status_automations_instance)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/settings/status-automations/{status_automation_id_}", project_id_,
    status_automation_id_
)
uuid project_id_{};
uuid status_automation_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/projects/{project_id}/authorization
DOODLE_HTTP_JWT_FUN(data_project_authorization)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/projects/{project_id_}/authorization", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/authorization/{authorization_id}
DOODLE_HTTP_JWT_FUN(data_project_authorization_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE(
    "/api/data/projects/{project_id_}/authorization/{authorization_id_}", project_id_, authorization_id_
)
uuid project_id_{};
uuid authorization_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/persons/{person_id}/change-password
DOODLE_HTTP_JWT_FUN(actions_persons_change_password)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/persons/{person_id_}/change-password", person_id_)
uuid person_id_{};
DOODLE_HTTP_FUN_END()
// /api/auth/change-password
DOODLE_HTTP_JWT_FUN(auth_change_password)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/auth/change-password")
DOODLE_HTTP_FUN_END()
// /api/movies/originals/preview-files/{preview_file_id}/download
DOODLE_HTTP_JWT_FUN(movies_originals_preview_files_download)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/movies/originals/preview-files/{preview_file_id_}/download", preview_file_id_)
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/fix-preview-files-thumbnails
DOODLE_HTTP_JWT_FUN(data_fix_preview_files_thumbnails)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/data/fix-preview-files-thumbnails")
DOODLE_HTTP_FUN_END()

//  /api/data/jobs
DOODLE_HTTP_JWT_FUN(data_jobs)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/jobs")
DOODLE_HTTP_FUN_END()

// /api/data/jobs/reassign
// DOODLE_HTTP_JWT_FUN(data_jobs_reassign)
// DOODLE_HTTP_FUN_OVERRIDE(post)
// DOODLE_HTTP_FUN_END()

// /api/data/jobs/{job_id}
DOODLE_HTTP_JWT_FUN(data_jobs_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/jobs/{job_id_}", job_id_)
uuid job_id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/jobs/{job_id}/log
DOODLE_HTTP_JWT_FUN(actions_jobs_log)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_ROUTE("/api/actions/jobs/{job_id_}/log", job_id_)
uuid job_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/computers
DOODLE_HTTP_JWT_FUN(data_computers)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/computers")
using session_data_ptr = std::shared_ptr<http::session_data>;
void websocket_callback(
    boost::beast::websocket::stream<http::tcp_stream_type> in_stream, http::session_data_ptr in_handle
) override;
[[nodiscard]] bool has_websocket() const override;
DOODLE_HTTP_FUN_END()
// /api/data/computers/{computer_id}
DOODLE_HTTP_JWT_FUN(data_computers_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/computers/{computer_id_}", computer_id_)
uuid computer_id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/projects/{project_id}/search
DOODLE_HTTP_JWT_FUN(actions_projects_search)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/actions/projects/{project_id_}/search", project_id_)
uuid project_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/ai_studio
DOODLE_HTTP_JWT_FUN(data_ai_studio)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_ROUTE("/api/data/ai_studio")
DOODLE_HTTP_FUN_END()
// /api/data/ai_studio/{id}
DOODLE_HTTP_JWT_FUN(data_ai_studio_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/ai_studio/{id_}", id_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/ai_studio/{id}/person/{id}
DOODLE_HTTP_JWT_FUN(data_ai_studio_instance_person_instance)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
DOODLE_HTTP_ROUTE("/api/data/ai_studio/{ai_studio_id_}/person/{person_id_}", ai_studio_id_, person_id_)
uuid ai_studio_id_{};
uuid person_id_{};
DOODLE_HTTP_FUN_END()
// /api/ue_plugins/version
DOODLE_HTTP_JWT_FUN(ue_plugins_version)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/ue_plugins/version")
DOODLE_HTTP_FUN_END()
// /api/ue_plugins/version/{version_major}.{version_minor}.{version_patch}
DOODLE_HTTP_JWT_FUN(ue_plugins_version_instance)
DOODLE_HTTP_ROUTE(
    "/api/ue_plugins/version/{version_major_}.{version_minor_}.{version_patch_}", version_major_, version_minor_,
    version_patch_
)
std::int32_t version_major_{};
std::int32_t version_minor_{};
std::int32_t version_patch_{};
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http