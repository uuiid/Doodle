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
      .reg_c<seedance2::doodle_ai_depth_estimation_video>()
      .reg_c<seedance2::doodle_ai_depth_estimation_file>()
      .reg_c<actions_projects_shots_import_frame_range>()
      .reg_c<actions_projects_sequences_casting_ue_assembly_harvest>()
      .reg_c<ai_light::shot_session_route>()
      .reg_c<ai_light::session_instance_route>()
      .reg_c<ai_light::reference_route>()
      .reg_c<ai_light::reference_instance_route>()
      .reg_c<ai_light::judge_route>()
      .reg_c<ai_light::iteration_submit_route>()
      .reg_c<ai_light::inference_route>()
      .reg_c<ai_light::candidate_begin_sequence_route>()
      .reg_c<ai_light::candidate_upload_frame_route>()
      .reg_c<ai_light::candidate_finish_sequence_route>()
      .reg_c<ai_light::candidate_movie_route>()
      .reg_c<ai_light::preset_route>()
      .reg_c<ai_light::preset_instance_route>()

      .reg_c<seedance2::seedance2_thumbnail>()
      .reg_c<seedance2::seedance2_pictures>()
      .reg_c<seedance2::seedance2_animation_waiting>()
      .reg_c<data_ai_studio>()
      .reg_c<data_ai_studio_instance>()
      .reg_c<data_ai_studio_instance_person_instance>()
      .reg_c<seedance2::seedance2_tokens>()
      .reg_c<seedance2::seedance2_tokens_person_instance>()
      .reg_c<seedance2::seedance2_tokens_person_all>()
      .reg_c<seedance2::seedance2_tokens_person_date_instance>()
      .reg_c<seedance2::seedance2_tokens_person_date_instance_day>()
      .reg_c<seedance2::seedance2_tokens_person_date_all>()
      .reg_c<seedance2::seedance2_tokens_person_date>()
      .reg_c<seedance2::seedance2_task_similarity>()
      .reg_c<seedance2::seedance2_subproject>()
      .reg_c<seedance2::seedance2_subproject_instance>()
      .reg_c<seedance2::seedance2_subproject_preview>()
      .reg_c<seedance2::seedance2_subproject_person_link>()
      .reg_c<seedance2::seedance2_subproject_ai_episode>()
      .reg_c<seedance2::seedance2_subproject_ai_episode_instance>()
      .reg_c<seedance2::seedance2_subproject_ai_episode_model_resolution_limit>()
      .reg_c<seedance2::seedance2_subproject_model_resolution_limit_instance>()
      .reg_c<seedance2::seedance2_subproject_entity>()
      .reg_c<seedance2::seedance2_subproject_ai_generate_entity>()
      .reg_c<seedance2::seedance2_subproject_ai_generate_entity_instance>()
      .reg_c<seedance2::seedance2_subproject_entity_preview>()
      .reg_c<seedance2::seedance2_subproject_task>()
      .reg_c<seedance2::seedance2_subproject_task_instance>()
      .reg_c<seedance2::seedance2_subproject_entity_reference>()
      .reg_c<seedance2::seedance2_subproject_reference_instance>()
      .reg_c<seedance2::seedance2_subproject_ai_category>()
      .reg_c<seedance2::seedance2_subproject_ai_category_instance>()
      .reg_c<seedance2::seedance2_subproject_ai_category_entity>()
      .reg_c<seedance2::canvas_>()
      .reg_c<seedance2::canvas_instance>()
      .reg_c<seedance2::canvas_node>()
      .reg_c<seedance2::canvas_node_instance>()
      .reg_c<seedance2::canvas_connection>()
      .reg_c<seedance2::canvas_connection_instance>()
      .reg_c<seedance2::canvas_person>()
      .reg_c<seedance2::canvas_person_instance>()
      .reg_c<seedance2::seedance2_task>()
      .reg_c<seedance2::seedance2_task_run>()
      .reg_c<seedance2::seedance2_task_date>()
      .reg_c<data_preview_files>()
      .reg_c<data_updata_logs>()
      .reg_c<data_production_specifications>()
      .reg_c<ue_plugins_version>()
      .reg_c<ue_plugins_version_instance>()
      
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
  (*l_rout_ptr)                  //
      .reg_c<local::local_setting>()  //
      .reg_c<local::video_thumbnail>()

      ;

  if (g_ctx().get<authorization>().is_valid())
    (*l_rout_ptr)
        .reg_c<local::task>()
        .reg_c<local::task_instance>()
        .reg_c<local::task_instance_log>()
        .reg_c<local::local_setting_tmp_dir_server_task>()
        .reg_c<local::task_inspect_instance>()
        .reg_c<local::task_instance_generate_uesk_file>()
        .reg_c<ai_train_animation>()
        .reg_c<ai_train_animation_settings>()
        .reg_c<local::actions_projects_shots_export_anim_fbx_local>()
        .reg_c<local::actions_projects_shots_update_sim_abc_local>()
        .reg_c<local::actions_project_sync_local>()
        .reg_c<local::actions_local_task_update_movie_files>()
        .reg_c<local::actions_local_task_update_ue_files>()
        .reg_c<local::actions_local_task_update_movie_compose>()
        .reg_c<local::tools_add_watermark>()
        .reg_c<local::actions_local_task_run>()
        .reg_c<local::actions_local_watch_file_maya_anim>()
        .reg_c<socket_io::socket_io_http>(l_sid_ctx)

        ;
  return l_rout_ptr;
}

http_route_ptr create_kitsu_epiboly_route(const FSys::path& in_root) {
  auto l_router  = std::make_shared<http_route>();
  auto l_sid_ctx = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->register_namespace("/events");
  (*l_router)
      .reg_c<doodle_tool_version>()
      .reg_c<local::local_setting>()  //
      // 外包
      .reg_c<config>()
      .reg_c<authenticated>()
      .reg_c<epiboly_user_context>()

      // 最后注册nodejs前端
      .reg_c<socket_io::socket_io_http>(l_sid_ctx)
      .reg_t<kitsu_front_end>(std::make_shared<kitsu_front_end_url_route_component>(), in_root)

      ;
  if (g_ctx().get<authorization>().is_valid())
    (*l_router).reg_c<epiboly_actions_projects_export_anim_fbx>();

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
