//
// Created by TD on 25-5-14.
//
#include <doodle_core/metadata/entity.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> playlists_entities_preview_files_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_person  = get_person(in_handle);
  auto l_entt_id = l_sql.get_by_uuid<entity>(in_arg->id_);
  l_person->is_project_access(l_entt_id.project_id_);
  nlohmann::json l_json{};
  for (auto&& [key, value] : l_sql.get_preview_files_for_entity(l_entt_id.uuid_id_))
    l_json[fmt::to_string(key)] = value;
  co_return in_handle->make_msg(l_json);
}

}  // namespace doodle::http