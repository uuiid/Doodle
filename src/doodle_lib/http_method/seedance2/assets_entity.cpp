#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/http/http_function.h"
#include "reg.h"
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json_fwd.hpp>
#include <sqlite_orm/sqlite_orm.h>
namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_group_entity, post) {
  auto l_json   = in_handle->get_json();
  auto l_entity = std::make_shared<sd2::assets_entity>();
  l_json.get_to(*l_entity);
  l_entity->name_         = "name";
  l_entity->group_id_     = group_id_;
  l_entity->user_id_      = person_.person_.uuid_id_;
  l_entity->ai_studio_id_ = person_.get_ai_studio_id();
  auto l_sql              = get_sqlite_database();
  co_await l_sql.install(l_entity);

  co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_instance, get) {
  auto l_sql    = get_sqlite_database();
  auto l_entity = l_sql.get_by_uuid<sd2::assets_entity>(entity_id_);
  DOODLE_CHICK_HTTP(l_entity.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足");
  co_return in_handle->make_msg(nlohmann::json{} = l_entity);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_instance, put) {
  auto l_sql    = get_sqlite_database();
  auto l_entity = std::make_shared<sd2::assets_entity>(l_sql.get_by_uuid<sd2::assets_entity>(entity_id_));
  DOODLE_CHICK_HTTP(l_entity->ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足");
  DOODLE_CHICK_HTTP(l_entity->uuid_id_ == person_.person_.uuid_id_ || person_.is_manager(), unauthorized, "权限不足");

  auto l_json = in_handle->get_json();
  l_json.get_to(*l_entity);
  co_await l_sql.update(l_entity);
  co_return in_handle->make_msg(nlohmann::json{} = *l_entity);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_instance, delete_) {
  auto l_sql    = get_sqlite_database();
  auto l_entity = l_sql.get_by_uuid<sd2::assets_entity>(entity_id_);
  DOODLE_CHICK_HTTP(l_entity.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足");
  DOODLE_CHICK_HTTP(l_entity.uuid_id_ == person_.person_.uuid_id_ || person_.is_manager(), unauthorized, "权限不足");

  co_await l_sql.remove<sd2::assets_entity>(entity_id_);
  co_return in_handle->make_msg_204();
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_entity_search, get) {
  std::string l_key;
  for (auto&& [key, value, has_value] : in_handle->url_.params()) {
    if (key == "key") l_key = value;
  }
  if (l_key.empty()) co_return in_handle->make_msg(nlohmann::json::array());

  l_key      = fmt::format("%{}%", l_key);
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::assets_entity>(where(
      like(&sd2::assets_entity::name_, l_key) && c(&sd2::assets_entity::ai_studio_id_) == person_.get_ai_studio_id()
  ));
  co_return in_handle->make_msg(l_vec);
}

}  // namespace doodle::http::seedance2