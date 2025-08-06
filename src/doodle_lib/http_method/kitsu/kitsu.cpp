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

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/socket_io.h>
#include <doodle_lib/core/socket_io/socket_io_ctx.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/file_association.h>
#include <doodle_lib/http_method/kitsu/epiboly.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/local/local.h>
#include <doodle_lib/http_method/other/other.h>
#include <doodle_lib/http_method/tool_version.h>
#include <doodle_lib/http_method/up_file.h>

#include "http_method/computing_time.h"
#include "http_method/dingding_attendance.h"
#include "http_method/model_library/model_library.h"
namespace doodle::http {

http_route_ptr create_kitsu_route_2(const FSys::path& in_root) {
  g_ctx().emplace<cache_manger>();

  auto l_router  = std::make_shared<http_route>();
  auto l_ctx     = g_ctx().get<kitsu_ctx_t>();
  auto l_sid_ctx = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->on("/events");
  (*l_router)
      // clang-format off
      // 我们自己的后端
      .reg_t<socket_io::socket_io_http>(R"(/socket\.io/)"_url, l_sid_ctx)
      .reg_t<doodle_data_asset_file_maya>("/api/doodle/data/asset/{}/file/maya"_url(&doodle_data_asset_file_image::id_))
      .reg_t<doodle_data_asset_file_ue>("/api/doodle/data/asset/{}/file/ue"_url(&doodle_data_asset_file_image::id_))
      .reg_t<doodle_data_asset_file_image>("/api/doodle/data/asset/{}/file/image"_url(&doodle_data_asset_file_image::id_))
      .reg_t<doodle_tool_version>("/api/doodle/tool/version"_url)
      .reg_t<doodle_file_association>("/api/doodle/file_association/{}"_url(&doodle_file_association::id_))
      .reg_t<doodle_file>("/api/doodle/file"_url)

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
      .reg_t<model_library::pictures_thumbnails>("/api/doodle/pictures/thumbnails/{}.png"_url(&model_library::pictures_thumbnails::id_), l_ctx.root_)

      /// 杂项
      .reg_t<other::deepseek_key>("/api/doodle/deepseek/key"_url)
      .reg_t<other::key_ji_meng>("/api/doodle/key/ji_meng"_url)

      /// 模拟 kitsu后端
      .reg_t<actions_user_notifications_mark_all_as_read>("/api/actions/user/notifications/mark-all-as-read"_url)
      .reg_t<actions_projects_tasks_comment_many>("/api/actions/projects/{}/tasks/comment-many"_url(&actions_projects_tasks_comment_many::id_))
      .reg_t<data_tasks_comments_ack>("/api/data/tasks/{}/comments/{}/ack"_url(&data_tasks_comments_ack::task_id_, &data_tasks_comments_ack::comment_id_))
      .reg_t<data_projects_team>("/api/data/projects/{}/team"_url(&data_projects_team::id_))
      .reg_t<auth_login>("/api/auth/login"_url)
      .reg_t<data_projects>("/api/data/projects"_url)
      .reg_t<data_project_settings_task_types>("/api/data/projects/{}/settings/task-types"_url(&data_project_settings_task_types::id_))
      .reg_t<data_project_settings_task_status>("/api/data/projects/{}/settings/task-status"_url(&data_project_settings_task_status::id_))
      .reg_t<data_project_settings_asset_types>("/api/data/projects/{}/settings/asset-types"_url(&data_project_settings_asset_types::id_))
      .reg_t<actions_create_tasks>("/api/actions/projects/{}/task-types/{}/assets/create-tasks"_url(&actions_create_tasks::project_id_, &actions_create_tasks::task_type_id_))
      .reg_t<projects_assets_new>("/api/data/projects/{}/asset-types/{}/assets/new"_url(&projects_assets_new::project_id_, &projects_assets_new::asset_type_id_))
      .reg_t<actions_tasks_comment>("/api/actions/tasks/{}/comment"_url(&actions_tasks_comment::id_))
      .reg_t<actions_tasks_comments_add_preview>("/api/actions/tasks/{}/comments/{}/add-preview"_url(&actions_tasks_comments_add_preview::task_id_, &actions_tasks_comments_add_preview::comment_id_))
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
      .reg_t<with_tasks>("/api/data/assets/with-tasks"_url)
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
      .reg_t<studios>("/api/data/studios"_url)
      .reg_t<task_types>("/api/data/task-types"_url)
      .reg_t<custom_actions>("/api/data/custom-actions"_url)
      .reg_t<status_automations>("/api/data/status-automations"_url)
      .reg_t<tasks_comments>("/api/data/tasks/{}/comments"_url(&tasks_comments::id_))
      .reg_t<user_context>("/api/data/user/context"_url)
      .reg_t<sequences_with_tasks>("/api/data/sequences/with-tasks"_url)
      .reg_t<pictures_thumbnails_organisations>("/api/pictures/thumbnails/organisations/{}.png"_url(&pictures_thumbnails_organisations::id_))
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
      .reg_t<actions_preview_files_update_annotations>("/api/data/actions/preview-files/{}/update-annotations"_url(&actions_preview_files_update_annotations::preview_file_id_))
      // 最后注册nodejs前端
      .reg_t<kitsu_front_end>(std::make_shared<kitsu_front_end_url_route_component>(), in_root)
      // clang-format on
      ;
  return l_router;
}

http_route_ptr create_kitsu_local_route() {
  auto l_rout_ptr = std::make_shared<http::http_route>();
  auto l_sid_ctx  = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->on("/socket.io/");
  (*l_rout_ptr)                                                      //
      .reg_t<local::local_setting>("/api/doodle/local_setting"_url)  //
      .reg_t<local::video_thumbnail>("/api/doodle/video/thumbnail"_url)

      ;

  if (g_ctx().get<authorization>().is_expire())
    (*l_rout_ptr)
        .reg_t<local::task>("/api/doodle/task"_url)
        .reg_t<local::task_instance>("/api/doodle/task/{}"_url(&local::task_instance::id_))
        .reg_t<local::task_instance_restart>("/api/doodle/task/{}/restart"_url(&local::task_instance_restart::id_))
        .reg_t<local::task_instance_log>("/api/doodle/task/{}/restart"_url(&local::task_instance_log::id_))

        .reg_t<socket_io::socket_io_http>(R"(/socket\.io/)"_url, l_sid_ctx)

        ;
  return l_rout_ptr;
}

http_route_ptr create_kitsu_epiboly_route(const FSys::path& in_root) {
  auto l_router = std::make_shared<http_route>();
  (*l_router)
      .reg_t<doodle_tool_version>("/api/doodle/tool/version"_url)

      // 外包
      .reg_t<epiboly_config>("/api/config"_url)
      .reg_t<epiboly_authenticated>("/api/auth/authenticated"_url)
      .reg_t<epiboly_user_context>("/api/data/user/context"_url)

      // 最后注册nodejs前端
      .reg_t<kitsu_front_end>(std::make_shared<kitsu_front_end_url_route_component>(), in_root)

      ;

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
  return "application/octet-stream";
}

}  // namespace kitsu

}  // namespace doodle::http