//
// Created by TD on 25-7-29.
//
#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include <doodle_core/metadata/entity.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> movies_low_preview_files::get(
    session_data_ptr in_handle
) {
  person_.check_not_outsourcer();

  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  person_.check_project_access(l_entt.project_id_);
  auto l_path = g_ctx().get<kitsu_ctx_t>().get_movie_lowdef_file(id_);

  co_return in_handle->make_msg(
      l_path, http_header_ctrl{.mine_type_ = kitsu::mime_type(l_path.extension()), .has_cache_control_ = true}
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> movies_originals_preview_files::get(
    session_data_ptr in_handle
) {
  person_.check_not_outsourcer();
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  auto l_path = g_ctx().get<kitsu_ctx_t>().get_movie_preview_file(id_);

  co_return in_handle->make_msg(
      l_path, http_header_ctrl{.mine_type_ = kitsu::mime_type(l_path.extension()), .has_cache_control_ = true}
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> movies_tiles_preview_files::get(
    session_data_ptr in_handle
) {
  person_.check_not_outsourcer();

  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<task>(l_sql.get_by_uuid<preview_file>(id_).task_id_);
  auto l_path = g_ctx().get<kitsu_ctx_t>().get_tiles_file_path(id_);

  co_return in_handle->make_msg(
      l_path, http_header_ctrl{.mine_type_ = kitsu::mime_type(l_path.extension()), .has_cache_control_ = true}
  );
}

}  // namespace doodle::http