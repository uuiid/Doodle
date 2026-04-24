
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/ai_studio.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "kitsu_reg_url.h"
#include <vector>

namespace doodle::http {
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio, post) {
  auto l_sql       = get_sqlite_database();
  auto l_json      = in_handle->get_json();
  auto l_ai_studio = std::make_shared<ai_studio>();
  l_json.get_to(*l_ai_studio);
  co_await l_sql.install(l_ai_studio);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ai_studio);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio, get) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  auto l_vec = sqlite_select::ai_studio_and_link_t_get_all();
  co_return in_handle->make_msg(nlohmann::json{} = l_vec);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance, get) {
  person_.check_producer();
  auto l_sql       = get_sqlite_database();
  auto l_ai_studio = l_sql.get_by_uuid<ai_studio>(id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_ai_studio);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance, put) {
  person_.check_producer();
  auto l_sql       = get_sqlite_database();
  auto l_ai_studio = std::make_shared<ai_studio>(l_sql.get_by_uuid<ai_studio>(id_));
  auto l_json      = in_handle->get_json();
  l_json.get_to(*l_ai_studio);
  co_await l_sql.update(l_ai_studio);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ai_studio);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance, delete_) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  l_sql.uuid_to_id<ai_studio>(id_);
  co_await l_sql.remove<ai_studio>(id_);
  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance_person_instance, post) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  l_sql.uuid_to_id<person>(person_id_);
  if (l_sql.is_person_ai_studio_connected(person_id_, ai_studio_id_)) co_return in_handle->make_msg_204();

  auto l_ai_studio_lk = std::make_shared<ai_studio_person_role_link>();
  auto l_json         = in_handle->get_json();
  l_json.get_to(*l_ai_studio_lk);
  co_await l_sql.install(l_ai_studio_lk);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ai_studio_lk);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance_person_instance, delete_) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  l_sql.uuid_to_id<person>(person_id_);
  if (!l_sql.is_person_ai_studio_connected(person_id_, ai_studio_id_)) co_return in_handle->make_msg_204();

  auto l_ai_studio_lk = l_sql.get_ai_studio_person_role_link(person_id_, ai_studio_id_);
  if (!l_ai_studio_lk) co_return in_handle->make_msg_204();
  co_await l_sql.remove<ai_studio_person_role_link>(l_ai_studio_lk->id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ai_studio_lk);
}
}  // namespace doodle::http