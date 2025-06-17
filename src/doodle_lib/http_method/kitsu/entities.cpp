//
// Created by TD on 25-6-16.
//
#include "doodle_core/metadata/entity.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

namespace doodle::http {
namespace {}
boost::asio::awaitable<boost::beast::http::message_generator> data_entities_put::callback(session_data_ptr in_handle) {
  auto l_p    = get_person(in_handle);
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_entt = std::make_shared<entity>(l_sql.get_by_uuid<entity>(in_handle->capture_->get_uuid()));

  in_handle->get_json().get_to(*l_entt);
  co_await l_sql.install(l_entt);
  co_return in_handle->make_msg(nlohmann::json{} = *l_entt);
}

}  // namespace doodle::http