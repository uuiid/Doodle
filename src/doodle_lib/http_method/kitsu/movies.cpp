//
// Created by TD on 25-7-29.
//
#include <doodle_core/metadata/entity.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> movies_low_preview_files_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr  = get_person(in_handle);
  auto l_uuid = in_arg->id_;
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<entity>(l_uuid);
  l_ptr->is_project_access(l_entt.project_id_);
  auto l_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "movies" / "lowdef" / FSys::split_uuid_path(fmt::format("{}.mp4", l_uuid));

  co_return in_handle->make_msg(l_path, kitsu::mime_type("video/mp4"));
}
boost::asio::awaitable<boost::beast::http::message_generator> movies_originals_preview_files_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr  = get_person(in_handle);
  auto l_uuid = in_arg->id_;
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = l_sql.get_by_uuid<entity>(l_uuid);
  l_ptr->is_project_access(l_entt.project_id_);
  auto l_path =
      g_ctx().get<kitsu_ctx_t>().root_ / "movies" / "previews" / FSys::split_uuid_path(fmt::format("{}.mp4", l_uuid));

  co_return in_handle->make_msg(l_path, kitsu::mime_type("video/mp4"));
}

}  // namespace doodle::http