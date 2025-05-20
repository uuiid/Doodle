//
// Created by TD on 25-5-14.
//
#include <doodle_core/metadata/entity.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> playlists_entities_preview_files_get::callback(
    session_data_ptr in_handle
) {
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_person  = get_person(in_handle);
  auto l_entt_id = l_sql.get_by_uuid<entity>(in_handle->capture_->get_uuid("id"));
  l_person->is_project_manager(l_entt_id.parent_id_);
  nlohmann::json l_json{};
  l_json = l_sql.get_preview_files_for_entity(l_entt_id.uuid_id_);
  co_return in_handle->make_msg(l_json);
}

}  // namespace doodle::http