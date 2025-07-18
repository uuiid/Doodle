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
#include <doodle_lib/http_method/kitsu/http_route_proxy.h>
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
  auto l_root_ptr = std::make_shared<FSys::path>(in_root);

  auto l_router   = std::make_shared<kitsu::http_route_proxy>();
  auto l_ctx      = g_ctx().get<kitsu_ctx_t>();
  l_router->reg_front_end(  // 前端
          std::make_shared<get_files_kitsu_front_end>(l_root_ptr),
          std::make_shared<get_files_head_kitsu_front_end>(l_root_ptr)
  );
  (*l_router)
      // 我们自己的后端
      .reg(std::make_shared<model_library::context_get>())

      .reg(std::make_shared<model_library::assets_get>())
      .reg(std::make_shared<model_library::assets_post>())
      .reg(std::make_shared<model_library::assets_put>())
      .reg(std::make_shared<model_library::assets_delete_>())

      .reg(std::make_shared<model_library::assets_tree_get>())
      .reg(std::make_shared<model_library::assets_tree_post>())
      .reg(std::make_shared<model_library::assets_tree_put>())
      .reg(std::make_shared<model_library::assets_tree_delete_>())

      .reg(std::make_shared<model_library::pictures_post>(l_ctx.root_))
      .reg(std::make_shared<model_library::pictures_get>(l_ctx.root_))
      .reg(std::make_shared<model_library::pictures_thumbnails_get>(l_ctx.root_))

      .reg(std::make_shared<model_library::assets_tree_link_post>())
      .reg(std::make_shared<model_library::assets_tree_link_delete_>())

      //
      .reg_t<doodle_file_association_get>()
      .reg_t<doodle_file_get>()
      .reg_t<doodle_tool_version_get>()

      .reg_t<dingding_attendance_get>()
      .reg_t<dingding_attendance_post>()
      .reg_t<dingding_attendance_custom_post>()
      .reg_t<dingding_attendance_custom_put>()
      .reg_t<dingding_attendance_custom_delete_>()

      .reg_t<computing_time_get>()
      .reg_t<computing_time_post>()
      .reg_t<computing_time_patch>()
      .reg_t<computing_time_delete_>()

      .reg_t<computing_time_add_post>()
      .reg_t<computing_time_custom_post>()
      .reg_t<computing_time_sort_post>()
      .reg_t<computing_time_average_post>()
      .reg_t<actions_user_notifications_mark_all_as_read_post>()

      .reg(std::make_shared<deepseek_key_get>())
      .reg(std::make_shared<other::key_ji_meng_get>())
      .reg(std::make_shared<up_file_asset_image_post>())
      .reg(std::make_shared<up_file_asset_maya_post>())
      .reg(std::make_shared<up_file_asset_ue_post>())

      /// 模拟 kitsu后端
      // post
      .reg(std::make_shared<auth_login_post>())
      .reg(std::make_shared<project_c_post>())
      .reg(std::make_shared<project_settings_task_types_post>())
      .reg(std::make_shared<project_settings_task_status_post>())
      .reg(std::make_shared<project_settings_asset_types_post>())
      .reg(std::make_shared<actions_create_tasks_post>())
      .reg(std::make_shared<projects_assets_new_post>())
      .reg(std::make_shared<task_comment_post>())
      .reg(std::make_shared<task_comment_add_preview_post>())
      .reg(std::make_shared<pictures_preview_files_post>())
      .reg(std::make_shared<data_task_status_links_post>())
      .reg(std::make_shared<auth_reset_password_post>())
      .reg(std::make_shared<data_person_post>())
      // put
      .reg(std::make_shared<data_comment_put>())
      .reg(std::make_shared<project_put>())
      .reg(std::make_shared<data_tasks_put>())
      .reg(std::make_shared<actions_persons_assign_put>())
      .reg(std::make_shared<actions_preview_files_set_main_preview_put>())
      .reg(std::make_shared<data_entities_put>())
      .reg(std::make_shared<auth_reset_password_put>())
      .reg(std::make_shared<data_person_put>())
      .reg(std::make_shared<actions_tasks_clear_assignation_put>())
      .reg(std::make_shared<data_user_notification_put>())
      // get
      .reg(std::make_shared<with_tasks_get>())
      .reg(std::make_shared<asset_details_get>())
      .reg(std::make_shared<project_all_get>())
      .reg(std::make_shared<project_get>())
      .reg(std::make_shared<shared_used_get>())
      .reg(std::make_shared<authenticated_get>())
      .reg(std::make_shared<organisations_get>())
      .reg(std::make_shared<config_get>())
      .reg(std::make_shared<data_user_tasks_get>())
      .reg(std::make_shared<data_user_done_tasks_get>())
      .reg(std::make_shared<data_user_time_spents_all_get>())
      .reg(std::make_shared<data_user_time_spents_get>())
      .reg(std::make_shared<tasks_to_check_get>())
      .reg(std::make_shared<person_day_off_get>())
      .reg(std::make_shared<person_all_get>())
      .reg(std::make_shared<person_day_off_all_get>())
      .reg(std::make_shared<person_time_spents_day_table_get>())
      .reg(std::make_shared<person_day_off_1_get>())
      .reg(std::make_shared<departments_get>())
      .reg(std::make_shared<studios_get>())
      .reg(std::make_shared<task_types_get>())
      .reg(std::make_shared<custom_actions_get>())
      .reg(std::make_shared<status_automations_get>())
      .reg(std::make_shared<tasks_comments_get>())
      .reg(std::make_shared<user_context_get>())
      .reg(std::make_shared<sequences_with_tasks_get>())
      .reg(std::make_shared<pictures_thumbnails_organisations_get>())
      .reg(std::make_shared<pictures_thumbnails_square_preview_files_get>())
      .reg(std::make_shared<pictures_thumbnails_preview_files_get>())
      .reg(std::make_shared<pictures_thumbnails_persons_get>())
      .reg(std::make_shared<playlists_entities_preview_files_get>())
      .reg(std::make_shared<pictures_originals_preview_files_download_get>())
      .reg(std::make_shared<pictures_previews_preview_files_get>())
      .reg(std::make_shared<data_attachment_files_file_get>())
      .reg(std::make_shared<data_project_schedule_items_task_types_get>())
      .reg(std::make_shared<data_project_schedule_items_get>())
      .reg(std::make_shared<data_project_milestones_get>())
      .reg(std::make_shared<data_tasks_open_tasks_get>())
      .reg_t<data_assets_cast_in_get>()
      .reg_t<data_entities_news_get>()
      .reg_t<pictures_originals_preview_files_get>()
      .reg_t<data_user_notifications_get>()

      // delete
      .reg(std::make_shared<task_comment_delete_>())
      .reg(std::make_shared<data_asset_delete_>())
      .reg(std::make_shared<project_settings_task_types_delete_>())
      .reg(std::make_shared<data_task_delete_>())

      ;

  auto l_sid_ctx = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->on("/events");
  socket_io::create_socket_io(*l_router, l_sid_ctx);
  return l_router;
}

http_route_ptr create_kitsu_local_route() {
  auto l_rout_ptr = std::make_shared<http::http_route>();
  auto l_sid_ctx  = std::make_shared<socket_io::sid_ctx>();
  l_sid_ctx->on("/socket.io/");
  socket_io::create_socket_io(*l_rout_ptr, l_sid_ctx);
  (*l_rout_ptr)                                           //
      .reg(std::make_shared<local::local_setting_get>())  //
      .reg(std::make_shared<local::local_setting_post>())
      .reg(std::make_shared<local::video_thumbnail_post>())
      .reg(std::make_shared<local::video_thumbnail_get>())

      ;

  if (g_ctx().get<authorization>().is_expire())
    (*l_rout_ptr)
        .reg(std::make_shared<local::task_get>())
        .reg(std::make_shared<local::task_post>())
        .reg(std::make_shared<local::task_patch>())
        .reg(std::make_shared<local::task_delete_>())
        .reg(std::make_shared<local::task_instance_get>())
        .reg(std::make_shared<local::task_instance_log_get>())
        .reg(std::make_shared<local::task_instance_restart_post>())

        ;
  return l_rout_ptr;
}

http_route_ptr create_kitsu_epiboly_route(const FSys::path& in_root) {
  auto l_router   = std::make_shared<kitsu::http_route_proxy>();
  auto l_root_ptr = std::make_shared<FSys::path>(in_root);
  l_router->reg_front_end(  // 前端
          std::make_shared<get_files_kitsu_front_end>(l_root_ptr),
          std::make_shared<get_files_head_kitsu_front_end>(l_root_ptr)
  );
  (*l_router)
      .reg_t<doodle_tool_version_get>()

      // 外包
      .reg_t<epiboly_config_get>()
      .reg_t<epiboly_authenticated_get>()
      .reg_t<epiboly_user_context_get>();

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