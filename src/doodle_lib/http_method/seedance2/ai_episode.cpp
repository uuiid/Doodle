#include "doodle_core/doodle_core_fwd.h"
#include <doodle_core/metadata/seedance2/ai_episode.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

// /api/seedance2/subproject/{subproject_id}/episodes
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_episode, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::ai_episode>())
                      .from<sd2::ai_episode>()
                      .where(c(&sd2::ai_episode::subproject_id_) == subproject_id_)()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_episode, post) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  auto l_episode = std::make_shared<sd2::ai_episode>();
  l_json.get_to(*l_episode);
  l_episode->subproject_id_ = subproject_id_;

  co_await l_sql.install(l_episode);

  co_return in_handle->make_msg(nlohmann::json{} = *l_episode);
}

// /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_episode_instance, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql     = get_sqlite_database();
  auto l_episode = l_sql.get_by_uuid<sd2::ai_episode>(episode_id_);

  co_return in_handle->make_msg(nlohmann::json{} = l_episode);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_episode_instance, put) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_json    = in_handle->get_json();

  auto l_episode = std::make_shared<sd2::ai_episode>(l_sql.get_by_uuid<sd2::ai_episode>(episode_id_));
  if (l_json.contains("name")) l_json.at("name").get_to(l_episode->name_);
  if (l_json.contains("description")) l_json.at("description").get_to(l_episode->description_);

  co_await l_sql.update(l_episode);

  co_return in_handle->make_msg(nlohmann::json{} = *l_episode);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_episode_instance, delete_) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_await l_sql.remove<sd2::ai_episode>(episode_id_);

  co_return in_handle->make_msg(nlohmann::json{{"episode_id", episode_id_}});
}

// /api/seedance2/subproject/{subproject_id}/episodes/{episode_id}/model-resolution-limit
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_episode_model_resolution_limit, get) {
  person_.check_subproject_access(subproject_id_);

  auto l_sql = get_sqlite_database();
  using namespace orm;
  auto l_result = select(l_sql)
                      .columns(object<sd2::ai_episode_model_resolution_limit>())
                      .from<sd2::ai_episode_model_resolution_limit>()
                      .where(c(&sd2::ai_episode_model_resolution_limit::ai_episode_id_) == episode_id_)()
                      .to_vector();
  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_ai_episode_model_resolution_limit, post) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql              = get_sqlite_database();
  auto l_json             = in_handle->get_json();

  auto l_limit            = std::make_shared<sd2::ai_episode_model_resolution_limit>();
  l_limit->ai_episode_id_ = episode_id_;
  l_json.get_to(*l_limit);

  co_await l_sql.install(l_limit);

  co_return in_handle->make_msg(nlohmann::json{} = *l_limit);
}

// /api/seedance2/subproject/{subproject_id}/model-resolution-limit/{limit_id}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_subproject_model_resolution_limit_instance, delete_) {
  person_.check_manager();
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  co_await l_sql.remove<sd2::ai_episode_model_resolution_limit>(limit_id_);

  co_return in_handle->make_msg(nlohmann::json{{"limit_id", limit_id_}});
}

}  // namespace doodle::http::seedance2
