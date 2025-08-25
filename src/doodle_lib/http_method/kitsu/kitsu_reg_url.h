//
// Created by TD on 25-4-1.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
// /api/auth/login
DOODLE_HTTP_FUN(auth_login)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/data/projects
DOODLE_HTTP_JWT_FUN(data_projects)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/data/projects/{}/settings/task-types
DOODLE_HTTP_JWT_FUN(data_project_settings_task_types)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/settings/task-status
DOODLE_HTTP_JWT_FUN(data_project_settings_task_status)
DOODLE_HTTP_FUN_OVERRIDE(post);
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/settings/asset-types
DOODLE_HTTP_JWT_FUN(data_project_settings_asset_types)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/comment
DOODLE_HTTP_JWT_FUN(actions_tasks_comment)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/actions/tasks/{task_id}/working-file
DOODLE_HTTP_JWT_FUN(actions_tasks_working_file)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/tasks/working-file-many
DOODLE_HTTP_JWT_FUN(actions_projects_tasks_working_file_many)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/working-file/scan-all
DOODLE_HTTP_JWT_FUN(actions_working_files_scan_all)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/shots
DOODLE_HTTP_JWT_FUN(data_project_shots)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/assets
DOODLE_HTTP_JWT_FUN(data_assets)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/output-types
DOODLE_HTTP_JWT_FUN(data_output_types)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/file-status
DOODLE_HTTP_JWT_FUN(data_file_status)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/comments/{comment_id}/add-preview
DOODLE_HTTP_JWT_FUN(actions_tasks_comments_add_preview)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid task_id_{};
uuid comment_id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/task-types/{task_type_id}/assets/create-tasks
DOODLE_HTTP_JWT_FUN(actions_create_tasks)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
uuid task_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/asset-types/{asset_type_id}/assets/new
DOODLE_HTTP_JWT_FUN(projects_assets_new)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
uuid asset_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/task-status-links
DOODLE_HTTP_JWT_FUN(data_task_status_links)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/data/persons
DOODLE_HTTP_JWT_FUN(data_person)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/auth/reset-password
DOODLE_HTTP_FUN(auth_reset_password)
void init();
auth_reset_password() { init(); }
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_END()
// /api/actions/user/notifications/mark-all-as-read
DOODLE_HTTP_JWT_FUN(actions_user_notifications_mark_all_as_read)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/tasks/comment-many
DOODLE_HTTP_JWT_FUN(actions_projects_tasks_comment_many)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{task_id}/comments/{comment_id}/ack
DOODLE_HTTP_JWT_FUN(data_tasks_comments_ack)
DOODLE_HTTP_FUN_OVERRIDE(post);
uuid task_id_{};
uuid comment_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/team
DOODLE_HTTP_JWT_FUN(data_projects_team)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/{task_id}/comments/{comment_id}/preview-files/{preview_file_id}
DOODLE_HTTP_JWT_FUN(actions_tasks_comments_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid task_id_{};
uuid comment_id_{};
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()

// /api/data/comments/{comment_id}
DOODLE_HTTP_JWT_FUN(data_comment)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/persons/{id}/assign
DOODLE_HTTP_JWT_FUN(actions_persons_assign)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{id}
DOODLE_HTTP_JWT_FUN(data_tasks)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/entities/{id}
DOODLE_HTTP_JWT_FUN(data_entities)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/preview-files/{id}/set-main-preview
DOODLE_HTTP_JWT_FUN(actions_preview_files_set_main_preview)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/persons/{id}
DOODLE_HTTP_JWT_FUN(data_person_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{}
DOODLE_HTTP_JWT_FUN(data_project_instance)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/tasks/clear-assignation
DOODLE_HTTP_JWT_FUN(actions_tasks_clear_assignation)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_END()
// /api/data/user/notifications/{id}
DOODLE_HTTP_JWT_FUN(data_user_notification)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/data/user/context
DOODLE_HTTP_JWT_FUN(user_context)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/auth/authenticated
DOODLE_HTTP_JWT_FUN(authenticated)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/organisations
DOODLE_HTTP_JWT_FUN(organisations)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/departments
DOODLE_HTTP_JWT_FUN(departments)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/studios
DOODLE_HTTP_JWT_FUN(studios)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/task-types
DOODLE_HTTP_JWT_FUN(task_types)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/custom-actions
DOODLE_HTTP_JWT_FUN(custom_actions)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/status-automations
DOODLE_HTTP_JWT_FUN(status_automations)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{task_id}/comments
DOODLE_HTTP_JWT_FUN(tasks_comments)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/assets/with-tasks
DOODLE_HTTP_JWT_FUN(data_assets_with_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/sequences/with-tasks
DOODLE_HTTP_JWT_FUN(sequences_with_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/shots/with-tasks
DOODLE_HTTP_JWT_FUN(data_shots_with_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/assets/{asset_id}
DOODLE_HTTP_JWT_FUN(asset_details)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/assets/shared-used
DOODLE_HTTP_JWT_FUN(shared_used)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/schedule-items/task-types
DOODLE_HTTP_JWT_FUN(data_project_schedule_items_task_types)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/schedule-items
DOODLE_HTTP_JWT_FUN(data_project_schedule_items)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/milestones
DOODLE_HTTP_JWT_FUN(data_project_milestones)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/user/tasks
DOODLE_HTTP_JWT_FUN(data_user_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/user/done-tasks
DOODLE_HTTP_JWT_FUN(data_user_done_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/user/tasks-to-check
DOODLE_HTTP_JWT_FUN(tasks_to_check)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/projects/all
DOODLE_HTTP_JWT_FUN(project_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/persons
DOODLE_HTTP_JWT_FUN(person_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/user/time-spents
DOODLE_HTTP_JWT_FUN(data_user_time_spents_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/user/time-spents/{date}
DOODLE_HTTP_JWT_FUN(data_user_time_spents)
DOODLE_HTTP_FUN_OVERRIDE(get)
chrono::year_month_day date_;
DOODLE_HTTP_FUN_END()

// /api/data/persons/{person_id}/day-offs/{date}
DOODLE_HTTP_JWT_FUN(person_day_off)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_;
chrono::year_month_day date_;
DOODLE_HTTP_FUN_END()
// /api/data/persons/{person_id}/day-offs
DOODLE_HTTP_JWT_FUN(person_day_off_all)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/persons/time-spents/day-table/{year}/{month}
DOODLE_HTTP_JWT_FUN(person_time_spents_day_table)
DOODLE_HTTP_FUN_OVERRIDE(get)
std::int32_t year_;
std::int32_t month_;
DOODLE_HTTP_FUN_END()
// /api/data/persons/day-offs/{year}/{month}
DOODLE_HTTP_JWT_FUN(person_day_off_1)
DOODLE_HTTP_FUN_OVERRIDE(get)
std::int32_t year_;
std::int32_t month_;
DOODLE_HTTP_FUN_END()
// /api/config
DOODLE_HTTP_JWT_FUN(config)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()

// /api/data/playlists/entities/{id}/preview-files
DOODLE_HTTP_JWT_FUN(playlists_entities_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/organisations/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_organisations)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails-square/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_square_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/thumbnails/persons/{id}
DOODLE_HTTP_JWT_FUN(pictures_thumbnails_persons)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/originals/preview-files/{id}/download
DOODLE_HTTP_JWT_FUN(pictures_originals_preview_files_download)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/pictures/originals/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_originals_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/pictures/previews/preview-files/{id}
DOODLE_HTTP_JWT_FUN(pictures_previews_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/attachment-files/{id}/file/{file_name}
DOODLE_HTTP_JWT_FUN(data_attachment_files_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_;
FSys::path file_name_;
DOODLE_HTTP_FUN_END()
// /api/data/tasks/open-tasks
DOODLE_HTTP_JWT_FUN(data_tasks_open_tasks)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/assets/<asset_id>/cast-in
DOODLE_HTTP_JWT_FUN(data_assets_cast_in)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/entities/<entity_id>/news
DOODLE_HTTP_JWT_FUN(data_entities_news)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/data/user/notifications
DOODLE_HTTP_JWT_FUN(data_user_notifications)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/tasks/{id}/full
DOODLE_HTTP_JWT_FUN(data_tasks_full)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/auth/logout
DOODLE_HTTP_JWT_FUN(auth_logout)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/movies/low/preview-files/{id}.mp4
DOODLE_HTTP_JWT_FUN(movies_low_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/movies/originals/preview-files/{id}.mp4
DOODLE_HTTP_JWT_FUN(movies_originals_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/movies/tiles/preview-files/{id}.png
DOODLE_HTTP_JWT_FUN(movies_tiles_preview_files)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()

// /api/data/tasks/{task_id}/comments/{comment_id}
DOODLE_HTTP_JWT_FUN(task_comment)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid task_id_;
uuid comment_id_;
DOODLE_HTTP_FUN_END()

// /api/data/projects/{project_id}/settings/task-types/{task_type_id}
DOODLE_HTTP_JWT_FUN(project_settings_task_types)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid project_id_;
uuid task_type_id_;
DOODLE_HTTP_FUN_END()

// /api/data/projects/{project_id}/team/{person_id}
DOODLE_HTTP_JWT_FUN(data_project_team_person)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid project_id_;
uuid person_id_;
DOODLE_HTTP_FUN_END()
// /api/data/projects/{}/sequences
DOODLE_HTTP_JWT_FUN(data_project_sequences)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/preview-files/<preview_file_id>/update-annotations
DOODLE_HTTP_JWT_FUN(actions_preview_files_update_annotations)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid preview_file_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/<project_id>/playlists/temp
DOODLE_HTTP_JWT_FUN(data_project_playlists_temp)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/sequences/{sequence_id}
DOODLE_HTTP_JWT_FUN(data_sequence_instance)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/<project_id>/task-types/<task_type_id>/create-tasks/<entity_type>
DOODLE_HTTP_JWT_FUN(actions_projects_task_types_create_tasks)
DOODLE_HTTP_FUN_OVERRIDE(post)
enum class url_entity_type_enum { episodes, sequence };
uuid project_id_{};
uuid task_type_id_{};
url_entity_type_enum entity_type_{};
DOODLE_HTTP_FUN_END()
// /api/data/task-type-links
DOODLE_HTTP_JWT_FUN(data_task_type_links)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()
// /api/data/shots/{shot_id}
DOODLE_HTTP_JWT_FUN(data_shot)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// /api/actions/projects/{project_id}/task-types/{task_type_id}/shots/create-tasks
DOODLE_HTTP_JWT_FUN(actions_projects_task_types_shots_create_tasks)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid project_id_{};
uuid task_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/sequences/all/casting
DOODLE_HTTP_JWT_FUN(data_project_sequences_all_casting)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid project_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/entities/{entity_id}/casting
DOODLE_HTTP_JWT_FUN(data_project_entities_casting)
DOODLE_HTTP_FUN_OVERRIDE(put)
uuid project_id_{};
uuid entity_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/sequences/{sequence_id}/casting
DOODLE_HTTP_JWT_FUN(data_project_sequences_casting)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid project_id_{};
uuid sequence_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/asset-types/{asset_type_id}/casting
DOODLE_HTTP_JWT_FUN(data_project_asset_types_casting)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid project_id_{};
uuid asset_type_id_{};
DOODLE_HTTP_FUN_END()
// /api/data/projects/{project_id}/playlists?sort_by=updated_at&page=1
DOODLE_HTTP_JWT_FUN(data_project_playlists)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid project_id_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http