//
// Created by TD on 25-7-29.
//
#include <doodle_core/metadata/entity.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> movies_low_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  person_.is_project_access(l_entt.project_id_);
  auto l_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "movies" / "lowdef" / FSys::split_uuid_path(fmt::format("{}.mp4", id_));

  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));
}
boost::asio::awaitable<boost::beast::http::message_generator> movies_originals_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  person_.is_project_access(l_entt.project_id_);
  auto l_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "movies" / "previews" / FSys::split_uuid_path(fmt::format("{}.mp4", id_));

  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));
}
boost::asio::awaitable<boost::beast::http::message_generator> movies_tiles_preview_files::get(
    session_data_ptr in_handle
) {
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  person_.is_project_access(l_entt.project_id_);
  auto l_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "pictures" / "tiles" / FSys::split_uuid_path(fmt::format("{}.png", id_));

  co_return in_handle->make_msg(l_path, kitsu::mime_type(l_path.extension()));
}

}  // namespace doodle::http