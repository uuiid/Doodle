
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/ai_studio.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

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
  auto l_vec = l_sql.get_all<ai_studio>();
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
  auto l_sql    = get_sqlite_database();
  auto l_person = std::make_shared<person>(l_sql.get_by_uuid<person>(person_id_));
  l_person->ai_studio_id_ = ai_studio_id_;
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance_person_instance, delete_) {
  person_.check_producer();
  auto l_sql    = get_sqlite_database();
  auto l_person = std::make_shared<person>(l_sql.get_by_uuid<person>(person_id_));
  l_person->ai_studio_id_ = {};
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}
}  // namespace doodle::http