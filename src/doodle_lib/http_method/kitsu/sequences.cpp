//
// Created by TD on 25-4-28.
//
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> sequences_with_tasks_get::callback(
    session_data_ptr in_handle
) {
  auto l_po      = get_person(in_handle);
  auto &l_sql    = g_ctx().get<sqlite_database>();
  auto l_type_id = l_sql.get_entity_type_by_name(std::string{doodle_config::entity_type_sequence});

  uuid l_project_uuid{};
  for (auto &&[key, value, has] : in_handle->url_.params())
    if (key == "project_id" && has) l_project_uuid = from_uuid_str(value);

  auto l_r = l_sql.get_entities_and_tasks(l_po->person_, l_project_uuid, l_type_id.uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_r);
}

}  // namespace doodle::http