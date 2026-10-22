//
// Created by TD on 24-8-21.
//
#include "kitsu.h"

#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/authorization.h>
#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/ai/ai_main.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/kitsu/computing_time.h>
#include <doodle_lib/http_method/kitsu/epiboly.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/local/local.h>
#include <doodle_lib/http_method/model_library/model_library.h>
#include <doodle_lib/http_method/other/other.h>
#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/http_method/tool_version.h>
#include <doodle_lib/http_method/up_file.h>
#include <doodle_lib/platform/win/register_file_type.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "ai/ai_main.h"
#include "kitsu/ai_light.h"
#include "kitsu/kitsu_reg_url.h"
#include "seedance2/reg.h"

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
      .reg_c<socket_io::socket_io_http>(l_sid_ctx)
      .reg_c<doodle_data_asset_file_maya>()
      .reg_c<doodle_data_asset_file_ue>()
      .reg_c<doodle_data_asset_file_image>()
      .reg_c<doodle_tool_version>()

      .reg_c<dingding_attendance_get>()
      .reg_c<dingding_attendance_create_post>()
      .reg_c<dingding_attendance_id_custom>()
      .reg_c<dingding_attendance_custom>()
      .reg_c<computing_time>()
      .reg_c<computing_time_add>()
      .reg_c<computing_time_custom>()
      .reg_c<computing_time_sort>()
      .reg_c<computing_time_average>()
      .reg_c<computing_time_patch>()
      .reg_c<computing_time_delete>()

      .reg_c<model_library::context>()
      .reg_c<model_library::model_library_assets>()
      .reg_c<model_library::model_library_assets_instance>()
      .reg_c<model_library::model_library_assets_tree>()
      .reg_c<model_library::model_library_assets_tree_instance>()
      .reg_c<model_library::assets_tree_link>()
      .reg_c<model_library::pictures_instance>(l_ctx.root_)
      .reg_c<model_library::pictures_instance_mp4>(l_ctx.root_)
      .reg_c<model_library::pictures_instance_no_ext>(l_ctx.root_)
      .reg_c<model_library::pictures_thumbnails>(l_ctx.root_)
      .reg_c<model_library::pictures_thumbnails_no_ext>(l_ctx.root_)

      /// 杂项
      .reg_c<other::deepseek_key>()
      .reg_c<other::key_ji_meng>()

      /// 模拟 kitsu后端
      .reg_c<actions_user_notifications_mark_all_as_read>()
      .reg_c<actions_projects_tasks_comment_many>()
      .reg_c<data_tasks_comments_ack>()
      .reg_c<data_projects_team>()
      .reg_c<auth_login>()
      .reg_c<auth_refresh_token>()
      .reg_c<data_projects>()
      .reg_c<data_project_settings_task_types>()
      .reg_c<data_project_settings_task_status>()
      .reg_c<data_project_settings_asset_types>()
      .reg_c<actions_create_tasks>()
      .reg_c<projects_assets_new>()
      .reg_c<actions_tasks_comment>()
      .reg_c<actions_tasks_comments_add_preview>()
      .reg_c<actions_tasks_create_review>()
      .reg_c<pictures_preview_files>()
      .reg_c<data_task_status_links>()
      .reg_c<auth_reset_password>()
      .reg_c<data_person>()
      .reg_c<actions_tasks_comments_preview_files>()
      .reg_c<data_comment>()
      .reg_c<data_project_instance>()
      .reg_c<data_tasks>()
      .reg_c<actions_persons_assign>()
      .reg_c<actions_preview_files_set_main_preview>()
      .reg_c<data_entities>()
      .reg_c<data_person_instance>()
      .reg_c<actions_tasks_clear_assignation>()
      .reg_c<data_user_notification>()
      .reg_c<data_assets_with_tasks>()
      .reg_c<asset_details>()
      .reg_c<project_all>()
      .reg_c<shared_used>()
      .reg_c<authenticated>()
      .reg_c<organisations>()
      .reg_c<config>()
      .reg_c<data_user_tasks>()
      .reg_c<data_user_done_tasks>()
      .reg_c<data_user_time_spents_all>()
      .reg_c<data_user_time_spents>()
      .reg_c<tasks_to_check>()
      .reg_c<person_day_off>()
      .reg_c<person_day_off_all>()
      .reg_c<person_time_spents_day_table>()
      .reg_c<person_day_off_1>()
      .reg_c<departments>()
      .reg_c<departments_instance>()
      .reg_c<studios>()
      .reg_c<studios_instance>()
      .reg_c<task_types>()
      .reg_c<custom_actions>()
      .reg_c<status_automations>()
      .reg_c<tasks_comments>()
      .reg_c<user_context>()
      .reg_c<sequences_with_tasks>()
      .reg_c<pictures_thumbnails_organisations_png>()
      .reg_c<pictures_thumbnails_organisations>()
      .reg_c<pictures_thumbnails_square_preview_files>()
      .reg_c<pictures_thumbnails_preview_files>()
      .reg_c<pictures_thumbnails_persons>()
      .reg_c<playlists_entities_preview_files>()
      .reg_c<pictures_originals_preview_files_download>()
      .reg_c<pictures_previews_preview_files>()
      .reg_c<data_attachment_files_file>()
      .reg_c<data_project_schedule_items_task_types>()
      .reg_c<data_project_schedule_items>()
      .reg_c<data_project_milestones>()
      .reg_c<data_tasks_open_tasks>()
      .reg_c<data_assets_cast_in>()
      .reg_c<data_entities_news>()
      .reg_c<pictures_originals_preview_files>()
      .reg_c<data_user_notifications>()
      .reg_c<data_tasks_full>()
      .reg_c<auth_logout>()
      .reg_c<data_shots_with_tasks>()
      .reg_c<movies_originals_preview_files>()
      .reg_c<movies_low_preview_files>()
      .reg_c<movies_tiles_preview_files>()
      .reg_c<task_comment>()
      .reg_c<project_settings_task_types>()
      .reg_c<data_project_team_person>()
      .reg_c<data_project_sequences>()
      .reg_c<actions_preview_files_update_annotations>()
      .reg_c<data_entities_preview_files>()

      .reg_c<data_project_playlists_temp>()
      .reg_c<data_assets>()
      .reg_c<data_file_status>()
      .reg_c<data_output_types>()
      .reg_c<actions_entity_working_file>()
      .reg_c<actions_projects_entity_working_file_many>()
      .reg_c<data_project_shots>()
      .reg_c<data_sequence_instance>()
      .reg_c<actions_projects_task_types_create_tasks>()
      .reg_c<data_task_type_links>()
      .reg_c<data_shot>()
      .reg_c<actions_projects_task_types_shots_create_tasks>()
      .reg_c<data_project_sequences_all_casting>()
      .reg_c<data_project_entities_casting>()
      .reg_c<data_project_sequences_casting>()
      .reg_c<data_project_asset_types_casting>()
      .reg_c<data_project_playlists>()
      .reg_c<data_playlists>()
      .reg_c<data_project_playlists_instance>()
      .reg_c<data_playlists_instance>()
      .reg_c<data_playlists_instance_entity_instance>()
      .reg_c<data_playlists_instance_shots>()
      .reg_c<actions_projects_casting_replace>()
      .reg_c<actions_projects_casting_copy>()
      .reg_c<data_entity_types_instance>()
      .reg_c<data_entity_types>()
      .reg_c<data_project_settings_status_automations>()
      .reg_c<data_task_status>()
      .reg_c<data_task_status_instance>()
      .reg_c<actions_tasks_modify_date_comment>()
      .reg_c<actions_projects_shots_working_file>()
      .reg_c<actions_projects_sequences_working_file>()
      .reg_c<actions_preview_files_compose_video>()
      .reg_c<actions_playlists_preview_files_create_review>()
      .reg_c<doodle_data_shots_file_maya>()
      .reg_c<doodle_data_shots_file_output>()
      .reg_c<doodle_data_shots_file_other>()
      .reg_c<doodle_data_shots_file_video>()
      .reg_c<doodle_data_shots_file_ue>()
      .reg_c<doodle_data_shots_file_auto_light>()
      .reg_c<doodle_data_shots_file_auto_light_movie>()
      .reg_c<actions_projects_shots_run_ue_assembly>()
      .reg_c<actions_projects_shots_run_export_anim_fbx>()
      .reg_c<actions_tasks_export_rig_sk>()
      .reg_c<actions_tasks_export_anim_fbx>()
      .reg_c<actions_tasks_sync_export_anim_fbx>()
      .reg_c<doodle_backup>()
      .reg_c<doodle_stop_server>()
      .reg_c<actions_tasks_sync>()
      .reg_c<actions_tasks_assets_update_ue>()
      .reg_c<data_project_settings_status_automations_instance>()
      .reg_c<actions_sequences_create_review_playlists>()
      .reg_c<data_project_authorization>()
      .reg_c<data_project_authorization_instance>()
      .reg_c<movies_originals_preview_files_download>()
      .reg_c<data_fix_preview_files_thumbnails>()
      .reg_c<actions_persons_change_password>()
      .reg_c<auth_change_password>()
      .reg_c<data_jobs>()
      .reg_c<data_jobs_instance>()
      .reg_c<actions_jobs_log>()
      .reg_c<data_computers>()
      .reg_c<data_computers_instance>()
      .reg_c<actions_projects_search>()
      .reg_t<seedance2::doodle_ai_depth_estimation_video>(
          "/api/seedance2/subproject/{}/entity/{}/depth"_url(
              &seedance2::doodle_ai_depth_estimation_video::subproject_id_,
              &seedance2::doodle_ai_depth_estimation_video::entity_id_))
      .reg_t<seedance2::doodle_ai_depth_estimation_file>(
          "/api/seedance2/depth/{}"_url(&seedance2::doodle_ai_depth_estimation_file::depth_id_))
      .reg_c<actions_projects_shots_import_frame_range>()
      .reg_c<actions_projects_sequences_casting_ue_assembly_harvest>()
      .reg_t<ai_light::shot_session_route>("/api/doodle/ai/light/shot/{}/session"_url(
        &ai_light::shot_session_route::entity_id_
      ))
      .reg_t<ai_light::session_instance_route>("/api/doodle/ai/light/session/{}"_url(
        &ai_light::session_instance_route::id_
      ))
      .reg_t<ai_light::reference_route>("/api/doodle/ai/light/session/{}/reference"_url(
        &ai_light::reference_route::session_id_
      ))
      .reg_t<ai_light::reference_instance_route>("/api/doodle/ai/light/reference/{}"_url(
        &ai_light::reference_instance_route::id_
      ))
      .reg_t<ai_light::judge_route>("/api/doodle/ai/light/session/{}/iteration/{}"_url(
        &ai_light::judge_route::session_id_, &ai_light::judge_route::iteration_id_
      ))
      .reg_t<ai_light::iteration_submit_route>("/api/doodle/ai/light/session/{}/iteration"_url(
        &ai_light::iteration_submit_route::session_id_
      ))
      .reg_t<ai_light::inference_route>("/api/doodle/ai/light/session/{}/inference"_url(
        &ai_light::inference_route::session_id_
      ))
      .reg_t<ai_light::candidate_begin_sequence_route>("/api/doodle/ai/light/candidate/{}/begin_sequence"_url(
        &ai_light::candidate_begin_sequence_route::candidate_id_
      ))
      .reg_t<ai_light::candidate_upload_frame_route>("/api/doodle/ai/light/candidate/{}/upload_frame"_url(
        &ai_light::candidate_upload_frame_route::candidate_id_
      ))
      .reg_t<ai_light::candidate_finish_sequence_route>("/api/doodle/ai/light/candidate/{}/finish_sequence"_url(
        &ai_light::candidate_finish_sequence_route::candidate_id_
      ))
      .reg_t<ai_light::candidate_movie_route>("/api/doodle/ai/light/candidate/{}/movie"_url(
        &ai_light::candidate_movie_route::id_
      ))
      .reg_t<ai_light::preset_route>("/api/doodle/ai/light/preset"_url)
      .reg_t<ai_light::preset_instance_route>("/api/doodle/ai/light/preset/{}"_url(
        &ai_light::preset_instance_route::id_
      ))

      .reg_t<seedance2::seedance2_thumbnail>("/api/seedance2/thumbnail/{}.png"_url(&seedance2::seedance2_thumbnail::id_))
      .reg_t<seedance2::seedance2_pictures>("/api/seedance2/pictures/{}{}"_url(
        &seedance2::seedance2_pictures::id_, &seedance2::seedance2_pictures::file_extension_
      ))
      .reg_t<seedance2::seedance2_animation_waiting>("/api/seedance2/animation/waiting.mp4"_url)
      .reg_t<data_ai_studio>("/api/data/ai_studio"_url)
      .reg_t<data_ai_studio_instance>("/api/data/ai_studio/{}"_url(&data_ai_studio_instance::id_))
      .reg_t<data_ai_studio_instance_person_instance>("/api/data/ai_studio/{}/person/{}"_url(
        &data_ai_studio_instance_person_instance::ai_studio_id_,
        &data_ai_studio_instance_person_instance::person_id_
      ))
      .reg_t<seedance2::seedance2_tokens>("/api/seedance2/tokens"_url)
      .reg_t<seedance2::seedance2_tokens_person_instance>("/api/seedance2/tokens/person/{}"_url(
        &seedance2::seedance2_tokens_person_instance::person_id_
      ))
      .reg_t<seedance2::seedance2_tokens_person_all>("/api/seedance2/tokens/person/all"_url)
      .reg_t<seedance2::seedance2_tokens_person_date_instance>("/api/seedance2/tokens/person/{}/date/{}-{}"_url(
        &seedance2::seedance2_tokens_person_date_instance::person_id_,
        &seedance2::seedance2_tokens_person_date_instance::date_start_,
        &seedance2::seedance2_tokens_person_date_instance::date_end_
      ))
      .reg_t<seedance2::seedance2_tokens_person_date_instance_day>("/api/seedance2/tokens/person/{}/date/{}"_url(
        &seedance2::seedance2_tokens_person_date_instance_day::person_id_,
        &seedance2::seedance2_tokens_person_date_instance_day::date_
      ))
      .reg_t<seedance2::seedance2_tokens_person_date_all>("/api/seedance2/tokens/person/date/{}-{}"_url(
        &seedance2::seedance2_tokens_person_date_all::date_start_,
        &seedance2::seedance2_tokens_person_date_all::date_end_
      ))
      .reg_t<seedance2::seedance2_tokens_person_date>(
        "/api/seedance2/tokens/person/date/{}"_url(
          &seedance2::seedance2_tokens_person_date::date_
        )
      )
      .reg_t<seedance2::seedance2_task_similarity>("/api/seedance2/task-similarity"_url)
      .reg_t<seedance2::seedance2_subproject>("/api/seedance2/subproject"_url)
      .reg_t<seedance2::seedance2_subproject_instance>("/api/seedance2/subproject/{}"_url(
        &seedance2::seedance2_subproject_instance::id_
      ))
      .reg_t<seedance2::seedance2_subproject_preview>("/api/seedance2/subproject/{}/preview"_url(
        &seedance2::seedance2_subproject_preview::id_
      ))
      .reg_t<seedance2::seedance2_subproject_person_link>("/api/seedance2/subproject/{}/person"_url(
        &seedance2::seedance2_subproject_person_link::subproject_id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_episode>("/api/seedance2/subproject/{}/episodes"_url(
        &seedance2::seedance2_subproject_ai_episode::subproject_id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_episode_instance>("/api/seedance2/subproject/{}/episodes/{}"_url(
        &seedance2::seedance2_subproject_ai_episode_instance::subproject_id_,
        &seedance2::seedance2_subproject_ai_episode_instance::episode_id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_episode_model_resolution_limit>(
        "/api/seedance2/subproject/{}/episodes/{}/model-resolution-limit"_url(
          &seedance2::seedance2_subproject_ai_episode_model_resolution_limit::subproject_id_,
          &seedance2::seedance2_subproject_ai_episode_model_resolution_limit::episode_id_
        )
      )
      .reg_t<seedance2::seedance2_subproject_model_resolution_limit_instance>(
        "/api/seedance2/subproject/{}/model-resolution-limit/{}"_url(
          &seedance2::seedance2_subproject_model_resolution_limit_instance::subproject_id_,
          &seedance2::seedance2_subproject_model_resolution_limit_instance::limit_id_
        )
      )
      .reg_t<seedance2::seedance2_subproject_entity>("/api/seedance2/subproject/{}/entity"_url(
        &seedance2::seedance2_subproject_entity::subproject_id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_generate_entity>("/api/seedance2/subproject/{}/episodes/{}/entity"_url(
        &seedance2::seedance2_subproject_ai_generate_entity::subproject_id_,
        &seedance2::seedance2_subproject_ai_generate_entity::episode_id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_generate_entity_instance>("/api/seedance2/subproject/{}/entity/{}"_url(
        &seedance2::seedance2_subproject_ai_generate_entity_instance::subproject_id_,
        &seedance2::seedance2_subproject_ai_generate_entity_instance::entity_id_
      ))
      .reg_t<seedance2::seedance2_subproject_entity_preview>("/api/seedance2/subproject/{}/entity/{}/preview"_url(
        &seedance2::seedance2_subproject_entity_preview::subproject_id_,
        &seedance2::seedance2_subproject_entity_preview::entity_id_
      ))
      .reg_t<seedance2::seedance2_subproject_task>("/api/seedance2/subproject/{}/entity/{}/task"_url(
        &seedance2::seedance2_subproject_task::subproject_id_,
        &seedance2::seedance2_subproject_task::entity_id_
      ))
      .reg_t<seedance2::seedance2_subproject_task_instance>("/api/seedance2/subproject/{}/task/{}"_url(
        &seedance2::seedance2_subproject_task_instance::subproject_id_,&seedance2::seedance2_subproject_task_instance::id_
      ))
      .reg_t<seedance2::seedance2_subproject_entity_reference>("/api/seedance2/subproject/{}/entity/{}/reference"_url(
        &seedance2::seedance2_subproject_entity_reference::subproject_id_,
        &seedance2::seedance2_subproject_entity_reference::entity_id_
      ))
      .reg_t<seedance2::seedance2_subproject_reference_instance>("/api/seedance2/subproject/{}/reference/{}"_url(
        &seedance2::seedance2_subproject_reference_instance::subproject_id_,
        &seedance2::seedance2_subproject_reference_instance::id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_category>("/api/seedance2/subproject/{}/category"_url(
        &seedance2::seedance2_subproject_ai_category::subproject_id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_category_instance>("/api/seedance2/subproject/{}/category/{}"_url(
        &seedance2::seedance2_subproject_ai_category_instance::subproject_id_,
        &seedance2::seedance2_subproject_ai_category_instance::category_id_
      ))
      .reg_t<seedance2::seedance2_subproject_ai_category_entity>("/api/seedance2/subproject/{}/category/{}/entity"_url(
        &seedance2::seedance2_subproject_ai_category_entity::subproject_id_,
        &seedance2::seedance2_subproject_ai_category_entity::category_id_
      ))
      .reg_t<seedance2::canvas_>("/api/seedance2/subproject/{}/canvas"_url(
        &seedance2::canvas_::subproject_id_
      ))
      .reg_t<seedance2::canvas_instance>("/api/seedance2/canvas/{}"_url(
        &seedance2::canvas_instance::canvas_id_
      ))
      .reg_t<seedance2::canvas_node>("/api/seedance2/canvas/{}/node"_url(
        &seedance2::canvas_node::canvas_id_
      ))
      .reg_c<seedance2::canvas_node_instance>()
      .reg_t<seedance2::canvas_connection>("/api/seedance2/canvas/{}/connection"_url(
        &seedance2::canvas_connection::canvas_id_
      ))
      .reg_t<seedance2::canvas_connection_instance>("/api/seedance2/canvas/{}/connection/{}"_url(
        &seedance2::canvas_connection_instance::canvas_id_,
        &seedance2::canvas_connection_instance::connection_id_
      ))
      .reg_t<seedance2::canvas_person>("/api/seedance2/canvas/{}/person"_url(
        &seedance2::canvas_person::canvas_id_
      ))
      .reg_t<seedance2::canvas_person_instance>("/api/seedance2/canvas_link/{}"_url(
        &seedance2::canvas_person_instance::canvas_link_id_
      ))
      .reg_t<seedance2::seedance2_task>("/api/seedance2/task"_url)
      .reg_t<seedance2::seedance2_task_run>("/api/seedance2/task/run"_url)
      .reg_t<seedance2::seedance2_task_date>("/api/seedance2/task/{}-{}"_url(
        &seedance2::seedance2_task_date::date_start_,
        &seedance2::seedance2_task_date::date_end_
      ))
      .reg_t<data_preview_files>("/api/data/preview-files/{}"_url(&data_preview_files::preview_file_id_))
      .reg_t<data_updata_logs>("/api/data/updata-logs"_url)
      .reg_t<data_production_specifications>("/api/data/production-specifications"_url)
      .reg_t<ue_plugins_version>("/api/ue-plugins/version"_url)
      .reg_t<ue_plugins_version_instance>("/api/ue-plugins/version/{}.{}.{}"_url(
        &ue_plugins_version_instance::version_major_,
        &ue_plugins_version_instance::version_minor_,
        &ue_plugins_version_instance::version_patch_
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
        .reg_t<ai_train_animation>("/api/doodle/ai/animation/train"_url)
        .reg_t<ai_train_animation_settings>("/api/doodle/ai/animation/train/settings"_url)
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
        .reg_t<local::actions_local_task_run>("/api/actions/local/task/run"_url)
        .reg_t<local::actions_local_watch_file_maya_anim>("/api/actions/local/watch_file/maya_anim"_url)
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
  if (l_ext == "md") return "text/markdown";
  return "application/octet-stream";
}

}  // namespace kitsu

}  // namespace doodle::http
