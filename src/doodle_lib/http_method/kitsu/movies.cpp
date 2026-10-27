//
// Created by TD on 25-7-29.
//
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(movies_low_preview_files, get) {
  person_.check_not_outsourcer();

  auto l_sql = get_sqlite_database();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  person_.check_project_access(l_entt.project_id_);
  auto l_path = g_kitsu_ctx().get_movie_lowdef_file(id_);
  auto l_mime = mime_type(l_path.extension());

  auto l_body = std::make_unique<http_body_file_t>(l_path, l_mime);
  l_body->set_cache_control();
  co_return l_body;
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(movies_originals_preview_files, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  auto l_path = g_kitsu_ctx().get_movie_preview_file(id_);
  auto l_mime = mime_type(l_path.extension());

  auto l_body = std::make_unique<http_body_file_t>(l_path, l_mime);
  l_body->set_cache_control();
  co_return l_body;
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(movies_tiles_preview_files, get) {
  person_.check_not_outsourcer();

  auto l_sql = get_sqlite_database();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  auto l_path = g_kitsu_ctx().get_tiles_file_path(id_);
  auto l_mime = mime_type(l_path.extension());

  auto l_body = std::make_unique<http_body_file_t>(l_path, l_mime);
  l_body->set_cache_control();
  co_return l_body;
}

}  // namespace doodle::http
