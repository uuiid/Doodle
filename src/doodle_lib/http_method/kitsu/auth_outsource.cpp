#include "doodle_core/metadata/person.h"
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/http_method/kitsu/kitsu_result.h>

#include "core/http/http_function.h"
#include <nlohmann/json_fwd.hpp>

namespace doodle::http {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization, get) {
  person_.check_producer();
  auto l_sql       = g_ctx().get<sqlite_database>();
  auto l_auth_list = l_sql.get_all<outsource_studio_authorization>();
  co_return in_handle->make_msg(nlohmann::json{} = l_auth_list);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization, post) {
  person_.check_producer();

  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_auth = std::make_shared<outsource_studio_authorization>();
  in_handle->get_json().get_to(*l_auth);
  co_await l_sql.install(l_auth);
  co_return in_handle->make_msg(nlohmann::json{} = *l_auth);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization_instance, get) {
  person_.check_producer();

  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_auth = l_sql.get_by_uuid<outsource_studio_authorization>(authorization_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_auth);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_authorization_instance, delete_) {
  person_.check_producer();

  auto l_sql = g_ctx().get<sqlite_database>();
  co_await l_sql.remove<outsource_studio_authorization>(authorization_id_);
  co_return in_handle->make_msg_204();
}
}  // namespace doodle::http