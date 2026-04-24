#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/kitsu_ctx_t.h>
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include "reg.h"
#include <memory>
#include <nlohmann/json_fwd.hpp>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_group, post) {
  auto l_json  = in_handle->get_json();

  auto l_group = std::make_shared<sd2::assets_group>();
  l_json.get_to(*l_group);
  if (l_group->name_.empty()) l_group->name_ = "group_";
  l_group->user_id_      = person_.person_.uuid_id_;
  l_group->ai_studio_id_ = person_.get_ai_studio_id();
  auto l_sql             = get_sqlite_database();
  co_await l_sql.install(l_group);

  co_return in_handle->make_msg(nlohmann::json{} = *l_group);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_group, get) {
  co_return in_handle->make_msg(
      nlohmann::json{} = sqlite_select::get_sd2_assets_group_for_ai_studio(person_.get_ai_studio_id())
  );
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_group_instance, get) {
  auto l_sql   = get_sqlite_database();
  auto l_group = l_sql.get_by_uuid<sd2::assets_group>(group_id_);
  DOODLE_CHICK_HTTP(l_group.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足");
  co_return in_handle->make_msg(nlohmann::json{} = l_group);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_group_instance, put) {
  auto l_sql   = get_sqlite_database();
  auto l_group = std::make_shared<sd2::assets_group>(l_sql.get_by_uuid<sd2::assets_group>(group_id_));
  DOODLE_CHICK_HTTP(l_group->ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足");
  DOODLE_CHICK_HTTP(l_group->uuid_id_ == person_.person_.uuid_id_, unauthorized, "权限不足");
  auto l_json = in_handle->get_json();
  l_json.get_to(*l_group);
  co_await l_sql.update(l_group);
  co_return in_handle->make_msg(nlohmann::json{} = *l_group);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_asset_library_group_instance, delete_) {
  auto l_sql   = get_sqlite_database();
  auto l_group = l_sql.get_by_uuid<sd2::assets_group>(group_id_);
  DOODLE_CHICK_HTTP(l_group.ai_studio_id_ == person_.get_ai_studio_id(), unauthorized, "权限不足");
  DOODLE_CHICK_HTTP(l_group.uuid_id_ == person_.person_.uuid_id_ || person_.is_manager(), unauthorized, "权限不足");

  using namespace sqlite_orm;
  DOODLE_CHICK_HTTP(
      sqlite_select::get_sd2_assets_count_for_assets_group(group_id_) == 0, bad_request, "分组下存在资产，无法删除"
  );

  co_await l_sql.remove<sd2::assets_group>(group_id_);
  co_return in_handle->make_msg(nlohmann::json{} = {{"id", group_id_}});
}

}  // namespace doodle::http::seedance2