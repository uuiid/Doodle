//
// Created by TD on 24-8-21.
//
#include "kitsu.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/authorization.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/platform/win/register_file_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "doodle_lib/http_method/dingding_attendance.h"
#include "doodle_lib/http_method/kitsu/computing_time.h"
#include "doodle_lib/http_method/model_library/model_library.h"
#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/ai/ai_main.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/kitsu/epiboly.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/local/local.h>
#include <doodle_lib/http_method/other/other.h>
#include <doodle_lib/http_method/tool_version.h>
#include <doodle_lib/http_method/up_file.h>

#include "kitsu/epiboly.h"
#include "kitsu/kitsu_reg_url.h"
#include "local/local.h"


namespace doodle::http {

http_route_ptr create_kitsu_route_2(const FSys::path& in_root) {
  g_ctx().emplace<cache_manger>();

  auto l_router  = std::make_shared<http_route>();
  auto l_ctx     = g_ctx().get<kitsu_ctx_t>();
  auto l_sid_ctx = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->register_namespace("/events");
  (*l_router)
      // clang-format off
      // 我们自己的后端
      .reg_t<socket_io::socket_io_http>(R"(/socket.io)"_url, l_sid_ctx)
      .reg_t<doodle_data_asset_file_maya>("/api/doodle/data/assets/{}/file/maya"_url(&doodle_data_asset_file_image::id_))
      .reg_t<doodle_data_asset_file_ue>("/api/doodle/data/assets/{}/file/ue"_url(&doodle_data_asset_file_image::id_))
      .reg_t<doodle_data_asset_file_image>("/api/doodle/data/assets/{}/file/image"_url(&doodle_data_asset_file_image::id_))
      .reg_t<doodle_tool_version>("/api/doodle/tool/version"_url)

      .reg_t<dingding_attendance_get>("/api/doodle/attendance/{}/{}"_url(&dingding_attendance_get::user_id_, &dingding_attendance_get::year_month_))
      .reg_t<dingding_attendance_create_post>("/api/doodle/attendance/{}"_url(&dingding_attendance_create_post::id_))
      .reg_t<dingding_attendance_id_custom>("/api/doodle/attendance/{}/custom"_url(&dingding_attendance_id_custom::id_))
      .reg_t<dingding_attendance_custom>("/api/doodle/attendance/custom/{}"_url(&dingding_attendance_custom::id_))
      .reg_t<computing_time>("/api/doodle/computing_time/{}/{}"_url(&computing_time::user_id_, &computing_time::year_month_))
      .reg_t<computing_time_add>("/api/doodle/computing_time/{}/{}/add"_url(&computing_time_add::user_id_, &computing_time_add::year_month_))
      .reg_t<computing_time_custom>("/api/doodle/computing_time/{}/{}/custom"_url(&computing_time_custom::user_id_, &computing_time_custom::year_month_))
      .reg_t<computing_time_sort>("/api/doodle/computing_time/{}/{}/sort"_url(&computing_time_sort::user_id_, &computing_time_sort::year_month_))
      .reg_t<computing_time_average>("/api/doodle/computing_time/{}/{}/average"_url(&computing_time_average::user_id_, &computing_time_average::year_month_))
      .reg_t<computing_time_patch>("/api/doodle/computing_time/{}/{}/{}"_url(&computing_time_patch::user_id_, &computing_time_patch::year_month_, &computing_time_patch::task_id_))
      .reg_t<computing_time_delete>("/api/doodle/computing_time/{}"_url(&computing_time_delete::id_))

      .reg_t<model_library::context>("/api/doodle/model_library/context"_url)
      .reg_t<model_library::model_library_assets>("/api/doodle/model_library/assets"_url)
      .reg_t<model_library::model_library_assets_instance>("/api/doodle/model_library/assets/{}"_url(&model_library::model_library_assets_instance::id_))
      .reg_t<model_library::model_library_assets_tree>("/api/doodle/model_library/assets_tree"_url)
      .reg_t<model_library::model_library_assets_tree_instance>("/api/doodle/model_library/assets_tree/{}"_url(&model_library::model_library_assets_tree_instance::id_))
      .reg_t<model_library::assets_tree_link>("/api/doodle/model_library/assets_tree/{}/assets/{}"_url(&model_library::assets_tree_link::id_, &model_library::assets_tree_link::assets_id_))
      .reg_t<model_library::pictures_instance>("/api/doodle/pictures/{}.png"_url(&model_library::pictures_instance::id_), l_ctx.root_)
      .reg_t<model_library::pictures_instance_mp4>("/api/doodle/pictures/{}.mp4"_url(&model_library::pictures_instance_mp4::id_), l_ctx.root_)
      .reg_t<model_library::pictures_instance>("/api/doodle/pictures/{}"_url(&model_library::pictures_instance::id_), l_ctx.root_)
      .reg_t<model_library::pictures_thumbnails>("/api/doodle/pictures/thumbnails/{}.png"_url(&model_library::pictures_thumbnails::id_), l_ctx.root_)
      .reg_t<model_library::pictures_thumbnails>("/api/doodle/pictures/thumbnails/{}"_url(&model_library::pictures_thumbnails::id_), l_ctx.root_)

      /// 杂项
      .reg_t<other::deepseek_key>("/api/doodle/deepseek/key"_url)
      .reg_t<other::key_ji_meng>("/api/doodle/key/ji_meng"_url)

      /// 模拟 kitsu后端
      .reg_t<actions_user_notifications_mark_all_as_read>("/api/actions/user/notifications/mark-all-as-read"_url)
      .reg_t<actions_projects_tasks_comment_many>("/api/actions/projects/{}/tasks/comment-many"_url(&actions_projects_tasks_comment_many::id_))
      .reg_t<data_tasks_comments_ack>("/api/data/tasks/{}/comments/{}/ack"_url(&data_tasks_comments_ack::task_id_, &data_tasks_comments_ack::comment_id_))
      .reg_t<data_projects_team>("/api/data/projects/{}/team"_url(&data_projects_team::id_))
      .reg_t<auth_login>("/api/auth/login"_url)
      .reg_t<auth_refresh_token>("/api/auth/refresh-token"_url)
      .reg_t<data_projects>("/api/data/projects"_url)
      .reg_t<data_project_settings_task_types>("/api/data/projects/{}/settings/task-types"_url(&data_project_settings_task_types::id_))
      .reg_t<data_project_settings_task_status>("/api/data/projects/{}/settings/task-status"_url(&data_project_settings_task_status::id_))
      .reg_t<data_project_settings_asset_types>("/api/data/projects/{}/settings/asset-types"_url(&data_project_settings_asset_types::id_))
      .reg_t<actions_create_tasks>("/api/actions/projects/{}/task-types/{}/assets/create-tasks"_url(&actions_create_tasks::project_id_, &actions_create_tasks::task_type_id_))
      .reg_t<projects_assets_new>("/api/data/projects/{}/asset-types/{}/assets/new"_url(&projects_assets_new::project_id_, &projects_assets_new::asset_type_id_))
      .reg_t<actions_tasks_comment>("/api/actions/tasks/{}/comment"_url(&actions_tasks_comment::id_))
      .reg_t<actions_tasks_comments_add_preview>("/api/actions/tasks/{}/comments/{}/add-preview"_url(&actions_tasks_comments_add_preview::task_id_, &actions_tasks_comments_add_preview::comment_id_))
      .reg_t<actions_tasks_create_review>("/api/actions/tasks/{}/create-review"_url(&actions_tasks_create_review::task_id_))
      .reg_t<pictures_preview_files>("/api/pictures/preview-files/{}"_url(&pictures_preview_files::id_))
      .reg_t<data_task_status_links>("/api/data/task-status-links"_url)
      .reg_t<auth_reset_password>("/api/auth/reset-password"_url)
      .reg_t<data_person>("/api/data/persons"_url)
      .reg_t<actions_tasks_comments_preview_files>("/api/actions/tasks/{}/comments/{}/preview-files/{}"_url(&actions_tasks_comments_preview_files::task_id_, &actions_tasks_comments_preview_files::comment_id_, &actions_tasks_comments_preview_files::preview_file_id_))
      .reg_t<data_comment>("/api/data/comments/{}"_url(&data_comment::id_))
      .reg_t<data_project_instance>("/api/data/projects/{}"_url(&data_project_instance::id_))
      .reg_t<data_tasks>("/api/data/tasks/{}"_url(&data_tasks::id_))
      .reg_t<actions_persons_assign>("/api/actions/persons/{}/assign"_url(&actions_persons_assign::id_))
      .reg_t<actions_preview_files_set_main_preview>("/api/actions/preview-files/{}/set-main-preview"_url(&actions_preview_files_set_main_preview::id_))
      .reg_t<data_entities>("/api/data/entities/{}"_url(&data_entities::id_))
      .reg_t<data_person_instance>("/api/data/persons/{}"_url(&data_person_instance::id_))
      .reg_t<actions_tasks_clear_assignation>("/api/actions/tasks/clear-assignation"_url)
      .reg_t<data_user_notification>("/api/data/user/notifications/{}"_url(&data_user_notification::id_))
      .reg_t<data_assets_with_tasks>("/api/data/assets/with-tasks"_url)
      .reg_t<asset_details>("/api/data/assets/{}"_url(&asset_details::id_))
      .reg_t<project_all>("/api/data/projects/all"_url)
      .reg_t<shared_used>("/api/data/projects/{}/assets/shared-used"_url(&shared_used::id_))
      .reg_t<authenticated>("/api/auth/authenticated"_url)
      .reg_t<organisations>("/api/data/organisations"_url)
      .reg_t<config>("/api/config"_url)
      .reg_t<data_user_tasks>("/api/data/user/tasks"_url)
      .reg_t<data_user_done_tasks>("/api/data/user/done-tasks"_url)
      .reg_t<data_user_time_spents_all>("/api/data/user/time-spents"_url)
      .reg_t<data_user_time_spents>("/api/data/user/time-spents/{}"_url(&data_user_time_spents::date_))
      .reg_t<tasks_to_check>("/api/data/user/tasks-to-check"_url)
      .reg_t<person_day_off>("/api/data/persons/{}/day-offs/{}"_url(&person_day_off::id_, &person_day_off::date_))
      .reg_t<person_day_off_all>("/api/data/persons/{}/day-offs"_url(&person_day_off_all::id_))
      .reg_t<person_time_spents_day_table>("/api/data/persons/time-spents/day-table/{}/{}"_url(&person_time_spents_day_table::year_, &person_time_spents_day_table::month_))
      .reg_t<person_day_off_1>("/api/data/persons/day-offs/{}/{}"_url(&person_day_off_1::year_, &person_day_off_1::month_))
      .reg_t<departments>("/api/data/departments"_url)
      .reg_t<departments_instance>("/api/data/departments/{}"_url(&departments_instance::id_))
      .reg_t<studios>("/api/data/studios"_url)
      .reg_t<task_types>("/api/data/task-types"_url)
      .reg_t<custom_actions>("/api/data/custom-actions"_url)
      .reg_t<status_automations>("/api/data/status-automations"_url)
      .reg_t<tasks_comments>("/api/data/tasks/{}/comments"_url(&tasks_comments::id_))
      .reg_t<user_context>("/api/data/user/context"_url)
      .reg_t<sequences_with_tasks>("/api/data/sequences/with-tasks"_url)
      .reg_t<pictures_thumbnails_organisations_png>("/api/pictures/thumbnails/organisations/{}.png"_url(&pictures_thumbnails_organisations_png::id_))
      .reg_t<pictures_thumbnails_organisations>("/api/pictures/thumbnails/organisations/{}"_url(&pictures_thumbnails_organisations::id_))
      .reg_t<pictures_thumbnails_square_preview_files>("/api/pictures/thumbnails-square/preview-files/{}.png"_url(&pictures_thumbnails_square_preview_files::id_))
      .reg_t<pictures_thumbnails_preview_files>("/api/pictures/thumbnails/preview-files/{}.png"_url(&pictures_thumbnails_preview_files::id_))
      .reg_t<pictures_thumbnails_persons>("/api/pictures/thumbnails/persons/{}.png"_url(&pictures_thumbnails_persons::id_))
      .reg_t<playlists_entities_preview_files>("/api/data/playlists/entities/{}/preview-files"_url(&playlists_entities_preview_files::id_))
      .reg_t<pictures_originals_preview_files_download>("/api/pictures/originals/preview-files/{}/download"_url(&pictures_originals_preview_files_download::id_))
      .reg_t<pictures_previews_preview_files>("/api/pictures/previews/preview-files/{}.png"_url(&pictures_previews_preview_files::id_))
      .reg_t<data_attachment_files_file>("/api/data/attachment-files/{}/file/{}"_url(&data_attachment_files_file::id_, &data_attachment_files_file::file_name_))
      .reg_t<data_project_schedule_items_task_types>("/api/data/projects/{}/schedule-items/task-types"_url(&data_project_schedule_items_task_types::id_))
      .reg_t<data_project_schedule_items>("/api/data/projects/{}/schedule-items"_url(&data_project_schedule_items::id_))
      .reg_t<data_project_milestones>("/api/data/projects/{}/milestones"_url(&data_project_milestones::id_))
      .reg_t<data_tasks_open_tasks>("/api/data/tasks/open-tasks"_url)
      .reg_t<data_assets_cast_in>("/api/data/assets/{}/cast-in"_url(&data_assets_cast_in::id_))
      .reg_t<data_entities_news>("/api/data/entities/{}/news"_url(&data_entities_news::id_))
      .reg_t<pictures_originals_preview_files>("/api/pictures/originals/preview-files/{}.png"_url(&pictures_originals_preview_files::id_))
      .reg_t<data_user_notifications>("/api/data/user/notifications"_url)
      .reg_t<data_tasks_full>("/api/data/tasks/{}/full"_url(&data_tasks_full::id_))
      .reg_t<auth_logout>("/api/auth/logout"_url)
      .reg_t<data_shots_with_tasks>("/api/data/shots/with-tasks"_url)
      .reg_t<movies_originals_preview_files>("/api/movies/originals/preview-files/{}.mp4"_url(&movies_originals_preview_files::id_))
      .reg_t<movies_low_preview_files>("/api/movies/low/preview-files/{}.mp4"_url(&movies_low_preview_files::id_))
      .reg_t<movies_tiles_preview_files>("/api/movies/tiles/preview-files/{}.png"_url(&movies_tiles_preview_files::id_))
      .reg_t<task_comment>("/api/data/tasks/{}/comments/{}"_url(&task_comment::task_id_, &task_comment::comment_id_))
      .reg_t<project_settings_task_types>("/api/data/projects/{}/settings/task-types/{}"_url(&project_settings_task_types::project_id_, &project_settings_task_types::task_type_id_))
      .reg_t<data_project_team_person>("/api/data/projects/{}/team/{}"_url(&data_project_team_person::project_id_, &data_project_team_person::person_id_))
      .reg_t<data_project_sequences>("/api/data/projects/{}/sequences"_url(&data_project_sequences::id_))
      .reg_t<actions_preview_files_update_annotations>("/api/actions/preview-files/{}/update-annotations"_url(&actions_preview_files_update_annotations::preview_file_id_))
      .reg_t<data_entities_preview_files>("/api/data/entities/{}/preview-files"_url(&data_entities_preview_files::entity_id_))

      .reg_t<data_project_playlists_temp>("/api/data/projects/{}/playlists/temp"_url(&data_project_playlists_temp::project_id_))
      .reg_t<data_assets>("/api/data/assets"_url)
      .reg_t<data_file_status>("/api/data/file-status"_url)
      .reg_t<data_output_types>("/api/data/output-types"_url)
      .reg_t<actions_entity_working_file>("/api/actions/entity/{}/working-file"_url(&actions_entity_working_file::id_))
      .reg_t<actions_projects_entity_working_file_many>("/api/actions/projects/{}/entity/working-file-many"_url(&actions_projects_entity_working_file_many::id_))
      .reg_t<data_project_shots>("/api/data/projects/{}/shots"_url(&data_project_shots::project_id_))
      .reg_t<data_sequence_instance>("/api/data/sequences/{}"_url(&data_sequence_instance::id_))
      .reg_t<actions_projects_task_types_create_tasks>("/api/actions/projects/{}/task-types/{}/create-tasks/{}"_url(
                                                   &actions_projects_task_types_create_tasks::project_id_,
                                                   &actions_projects_task_types_create_tasks::task_type_id_,
                                                   &actions_projects_task_types_create_tasks::entity_type_))
      .reg_t<data_task_type_links>("/api/data/task-type-links"_url)
      .reg_t<data_shot>("/api/data/shots/{}"_url(&data_shot::id_))
      .reg_t<actions_projects_task_types_shots_create_tasks>("/api/actions/projects/{}/task-types/{}/shots/create-tasks"_url(
                                                   &actions_projects_task_types_shots_create_tasks::project_id_,
                                                   &actions_projects_task_types_shots_create_tasks::task_type_id_))
      .reg_t<data_project_sequences_all_casting>("/api/data/projects/{}/sequences/all/casting"_url(&data_project_sequences_all_casting::project_id_))
      .reg_t<data_project_entities_casting>("/api/data/projects/{}/entities/{}/casting"_url(
                                          &data_project_entities_casting::project_id_,
                                          &data_project_entities_casting::entity_id_
                                          ))
      .reg_t<data_project_sequences_casting>("/api/data/projects/{}/sequences/{}/casting"_url(
                                          &data_project_sequences_casting::project_id_,
                                          &data_project_sequences_casting::sequence_id_))
      .reg_t<data_project_asset_types_casting>("/api/data/projects/{}/asset-types/{}/casting"_url(
                                          &data_project_asset_types_casting::project_id_,
                                          &data_project_asset_types_casting::asset_type_id_))
      .reg_t<data_project_playlists>("/api/data/projects/{}/playlists"_url(&data_project_playlists::project_id_))
      .reg_t<data_playlists>("/api/data/playlists"_url)
      .reg_t<data_project_playlists_instance>("/api/data/projects/{}/playlists/{}"_url(
                                          &data_project_playlists_instance::project_id_,
                                          &data_project_playlists_instance::playlist_id_))
      .reg_t<data_playlists_instance>("/api/data/playlists/{}"_url(&data_playlists_instance::id_))
      .reg_t<data_playlists_instance_entity_instance>("/api/data/playlists/{}/entities/{}"_url(
        &data_playlists_instance_entity_instance::playlist_id_,
        &data_playlists_instance_entity_instance::entity_id_
      ))
      .reg_t<data_playlists_instance_shots>("/api/data/playlists/{}/shots/{}"_url(
        &data_playlists_instance_shots::playlist_id_,
        &data_playlists_instance_shots::shot_id_
      ))
      .reg_t<actions_projects_casting_replace>("/api/actions/projects/{}/casting/replace"_url(&actions_projects_casting_replace::project_id_))
      .reg_t<data_entity_types_instance>("/api/data/entity-types/{}"_url(&data_entity_types_instance::id_))
      .reg_t<model_library::ai_image>("/api/doodle/ai_image"_url)
      .reg_t<model_library::ai_image_instance>("/api/doodle/ai_image/{}"_url(&model_library::ai_image_instance::id_))
      .reg_t<data_project_settings_status_automations>("/api/data/projects/{}/settings/status-automations"_url(&data_project_settings_status_automations::id_))
      .reg_t<data_task_status>("/api/data/task-status"_url) 
      .reg_t<data_task_status_instance>("/api/data/task-status/{}"_url(&data_task_status_instance::id_)) 
      .reg_t<actions_tasks_modify_date_comment>("/api/actions/tasks/{}/modify-date-comment"_url(&actions_tasks_modify_date_comment::id_))
      .reg_t<actions_projects_shots_working_file>("/api/actions/projects/{}/shots/{}/working-file"_url(
        &actions_projects_shots_working_file::project_id_,
        &actions_projects_shots_working_file::id_
      ))
      .reg_t<actions_projects_sequences_working_file>("/api/actions/projects/{}/sequences/{}/working-file"_url(
        &actions_projects_sequences_working_file::project_id_,
        &actions_projects_sequences_working_file::id_
      ))
      .reg_t<actions_preview_files_compose_video>("/api/actions/preview-files/{}/compose-video"_url(
        &actions_preview_files_compose_video::preview_file_id_
      ))
      .reg_t<actions_playlists_preview_files_create_review>("/api/actions/playlists/{}/preview-files/{}/create-review"_url(
        &actions_playlists_preview_files_create_review::playlist_id_,
        &actions_playlists_preview_files_create_review::preview_file_id_
      ))
      .reg_t<doodle_data_shots_file_maya>("/api/doodle/data/shots/{}/file/maya"_url(&doodle_data_shots_file_maya::id_))
      .reg_t<doodle_data_shots_file_output>("/api/doodle/data/shots/{}/file/output"_url(&doodle_data_shots_file_output::id_))
      .reg_t<doodle_data_shots_file_other>("/api/doodle/data/shots/{}/file/other"_url(&doodle_data_shots_file_other::id_))
      .reg_t<doodle_data_shots_file_video>("/api/doodle/data/shots/{}/file/video"_url(&doodle_data_shots_file_video::id_))
      .reg_t<doodle_data_shots_file_ue>("/api/doodle/data/shots/{}/file/ue"_url(&doodle_data_shots_file_ue::id_))
      .reg_t<actions_projects_shots_run_ue_assembly>("/api/actions/projects/{}/shots/{}/run-ue-assembly"_url(
        &actions_projects_shots_run_ue_assembly::project_id_,
        &actions_projects_shots_run_ue_assembly::id_
      ))
      .reg_t<actions_tasks_export_rig_sk>("/api/actions/tasks/{}/export-rig-sk"_url(&actions_tasks_export_rig_sk::task_id_))
      .reg_t<actions_tasks_export_anim_fbx>("/api/actions/tasks/{}/export-anim-fbx"_url(&actions_tasks_export_anim_fbx::task_id_))
      .reg_t<doodle_backup>("/api/doodle/backup"_url)
      .reg_t<doodle_stop_server>("/api/doodle/stop-server"_url)
      .reg_t<actions_tasks_sync>("/api/actions/tasks/{}/sync"_url(&actions_tasks_sync::task_id_))
      .reg_t<actions_tasks_assets_update_ue>("/api/actions/tasks/{}/assets/update/ue"_url(&actions_tasks_assets_update_ue::task_id_))
      .reg_t<data_project_settings_status_automations_instance>("/api/data/projects/{}/settings/status-automations/{}"_url(
        &data_project_settings_status_automations_instance::project_id_, &data_project_settings_status_automations_instance::status_automation_id_
      ))
      .reg_t<actions_sequences_create_review_playlists>("/api/actions/sequences/{}/create-review/playlists"_url(
        &actions_sequences_create_review_playlists::sequence_id_
      ))
      .reg_t<data_project_authorization>("/api/data/projects/{}/authorization"_url(&data_project_authorization::project_id_))
      .reg_t<data_project_authorization_instance>("/api/data/projects/{}/authorization/{}"_url(
        &data_project_authorization_instance::project_id_,
        &data_project_authorization_instance::authorization_id_
      ))

      // 最后注册nodejs前端
      .reg_t<kitsu_front_end>(std::make_shared<kitsu_front_end_url_route_component>(), in_root)
      // clang-format on
      ;
  return l_router;
}

http_route_ptr create_kitsu_local_route() {
  auto l_rout_ptr = std::make_shared<http::http_route>();
  auto l_sid_ctx  = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->register_namespace("/events");
  (*l_rout_ptr)                                                      //
      .reg_t<local::local_setting>("/api/doodle/local_setting"_url)  //
      .reg_t<local::video_thumbnail>("/api/doodle/video/thumbnail"_url)

      ;

  if (g_ctx().get<authorization>().is_valid())
    (*l_rout_ptr)
        .reg_t<local::task>("/api/doodle/task"_url)
        .reg_t<local::task_instance>("/api/doodle/task/{}"_url(&local::task_instance::id_))
        .reg_t<local::task_instance_log>("/api/doodle/task/{}/log"_url(&local::task_instance_log::id_))
        .reg_t<local::local_setting_tmp_dir_server_task>("/api/doodle/local_setting/tmp_dir/server_task"_url)
        .reg_t<local::task_inspect_instance>("/api/doodle/task/{}/inspect"_url(&local::task_inspect_instance::id_))
        .reg_t<local::task_instance_generate_uesk_file>(
            "/api/doodle/task/{}/generate_uesk_file"_url(&local::task_instance_generate_uesk_file::id_)
        )
        .reg_t<ai_train_binding_weights>("/api/doodle/ai/train-binding-weights"_url)
        .reg_t<local::actions_projects_shots_run_ue_assembly_local>(
            "/api/actions/projects/{}/shots/{}/run-ue-assembly"_url(
                &local::actions_projects_shots_run_ue_assembly_local::project_id_,
                &local::actions_projects_shots_run_ue_assembly_local::id_
            )
        )
        .reg_t<local::actions_projects_shots_export_anim_fbx_local>(
            "/api/actions/projects/{}/shots/{}/export-anim-fbx"_url(
                &local::actions_projects_shots_export_anim_fbx_local::project_id_,
                &local::actions_projects_shots_export_anim_fbx_local::id_
            )
        )
        .reg_t<local::actions_projects_shots_update_sim_abc_local>(
            "/api/actions/projects/{}/shots/{}/update-sim-abc"_url(
                &local::actions_projects_shots_update_sim_abc_local::project_id_,
                &local::actions_projects_shots_update_sim_abc_local::id_
            )
        )
        .reg_t<local::actions_project_sync_local>(
            "/api/actions/project/{}/sync"_url(&local::actions_project_sync_local::project_id_)
        )
        .reg_t<local::actions_local_task_update_movie_files>(
            "/api/actions/local/task/{}/update/movie"_url(&local::actions_local_task_update_movie_files::id_)
        )
        .reg_t<local::actions_local_task_update_ue_files>(
            "/api/actions/local/task/{}/update/ue"_url(&local::actions_local_task_update_ue_files::id_)
        )
        .reg_t<local::actions_local_task_update_movie_compose>(
            "/api/actions/local/task/{}/update/movie/compose"_url(&local::actions_local_task_update_movie_compose::id_)
        )
        .reg_t<local::tools_add_watermark>("/api/actions/tools/add-watermark"_url)
        .reg_t<socket_io::socket_io_http>(R"(/socket.io)"_url, l_sid_ctx)

        ;
  return l_rout_ptr;
}

http_route_ptr create_kitsu_epiboly_route(const FSys::path& in_root) {
  auto l_router  = std::make_shared<http_route>();
  auto l_sid_ctx = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->register_namespace("/events");
  (*l_router)
      .reg_t<doodle_tool_version>("/api/doodle/tool/version"_url)
      .reg_t<local::local_setting>("/api/doodle/local_setting"_url)  //
      // 外包
      .reg_t<config>("/api/config"_url)
      .reg_t<authenticated>("/api/auth/authenticated"_url)
      .reg_t<epiboly_user_context>("/api/data/user/context"_url)

      // 最后注册nodejs前端
      .reg_t<socket_io::socket_io_http>(R"(/socket.io)"_url, l_sid_ctx)
      .reg_t<kitsu_front_end>(std::make_shared<kitsu_front_end_url_route_component>(), in_root)

      ;
  if (g_ctx().get<authorization>().is_valid())
    (*l_router).reg_t<epiboly_actions_projects_export_anim_fbx>(
        "/api/actions/projects/{}/export-anim-fbx"_url(&epiboly_actions_projects_export_anim_fbx::project_id_)
    );

  return l_router;
}

namespace kitsu {

uuid get_url_project_id(const boost::urls::url& in_url) {
  auto l_q = in_url.query();
  if (auto l_it = l_q.find("project_id"); l_it != l_q.npos) {
    auto l_str = l_q.substr(l_it + 11, l_q.find('&', l_it) - l_it - 11);
    return boost::lexical_cast<uuid>(l_str);
  }
  return {};
}
doodle::details::assets_type_enum conv_assets_type_enum(const std::string& in_name) {
  if (in_name == "角色") {
    return doodle::details::assets_type_enum::character;
  } else if (in_name == "场景") {
    return doodle::details::assets_type_enum::scene;
  } else if (in_name == "道具") {
    return doodle::details::assets_type_enum::prop;
  } else if (in_name == "绑定") {
    return doodle::details::assets_type_enum::rig;
  } else if (in_name == "动画") {
    return doodle::details::assets_type_enum::animation;
  } else if (in_name == "特效") {
    return doodle::details::assets_type_enum::vfx;
  } else if (in_name == "地编模型") {
    return doodle::details::assets_type_enum::scene;
  } else if (in_name == "地编") {
    return doodle::details::assets_type_enum::scene;
  }
  return doodle::details::assets_type_enum::other;
}

std::string_view mime_type(const FSys::path& in_ext) {
  auto l_ext = in_ext.generic_string();
  if (l_ext.empty()) return "application/octet-stream";
  if (l_ext.front() == '.') l_ext.erase(0, 1);
  if (l_ext == "htm") return "text/html";
  if (l_ext == "html") return "text/html";
  if (l_ext == "php") return "text/html";
  if (l_ext == "css") return "text/css";
  if (l_ext == "txt") return "text/plain";
  if (l_ext == "js") return "application/javascript";
  if (l_ext == "json") return "application/json";
  if (l_ext == "xml") return "application/xml";
  if (l_ext == "swf") return "application/x-shockwave-flash";
  if (l_ext == "flv") return "video/x-flv";
  if (l_ext == "png") return "image/png";
  if (l_ext == "jpe") return "image/jpeg";
  if (l_ext == "jpeg") return "image/jpeg";
  if (l_ext == "jpg") return "image/jpeg";
  if (l_ext == "gif") return "image/gif";
  if (l_ext == "bmp") return "image/bmp";
  if (l_ext == "ico") return "image/vnd.microsoft.icon";
  if (l_ext == "tiff") return "image/tiff";
  if (l_ext == "tif") return "image/tiff";
  if (l_ext == "svg") return "image/svg+xml";
  if (l_ext == "svgz") return "image/svg+xml";
  if (l_ext == "map") return "application/json";
  if (l_ext == "exe") return "application/octet-stream";
  if (l_ext == "m3u8") return "application/vnd.apple.mpegurl";
  if (l_ext == "mp4") return "video/mp4";
  if (l_ext == "webm") return "video/webm";
  if (l_ext == "mkv") return "video/x-matroska";
  if (l_ext == "ts") return "video/mp2t";
  if (l_ext == "pdf") return "application/pdf";
  if (l_ext == "zip") return "application/zip";
  if (l_ext == "rar") return "application/x-rar-compressed";
  if (l_ext == "7z") return "application/x-7z-compressed";
  if (l_ext == "tar") return "application/x-tar";
  if (l_ext == "log") return "text/plain";
  if (l_ext == "mov") return "video/quicktime";
  return "application/octet-stream";
}

}  // namespace kitsu

}  // namespace doodle::http