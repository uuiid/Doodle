#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/seedance2/ai_generate_classification.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

// /api/seedance2/subproject/{subproject_id}/classification
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_classification, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_classification          = std::make_shared<sd2::ai_generate_classification>();
  l_json.get_to(*l_classification);
  l_classification->subproject_id_ = subproject_id_;

  co_await l_sql.install(l_classification);

  co_return in_handle->make_msg(nlohmann::json{} = *l_classification);
}

// /api/seedance2/subproject/{subproject_id}/classification/{classification_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_classification_instance, get) {
  auto l_sql            = get_sqlite_database();
  auto l_classification = l_sql.get_by_uuid<sd2::ai_generate_classification>(classification_id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_classification);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_classification_instance, put) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_classification = std::make_shared<sd2::ai_generate_classification>(
      l_sql.get_by_uuid<sd2::ai_generate_classification>(classification_id_)
  );
  if (l_json.contains("name")) l_json.at("name").get_to(l_classification->name_);
  if (l_json.contains("description")) l_json.at("description").get_to(l_classification->description_);

  co_await l_sql.update(l_classification);

  co_return in_handle->make_msg(nlohmann::json{} = *l_classification);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_generate_classification_instance, delete_) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_await l_sql.remove<sd2::ai_generate_classification>(classification_id_);

  co_return in_handle->make_msg(nlohmann::json{{"classification_id", classification_id_}});
}

}  // namespace doodle::http::seedance2