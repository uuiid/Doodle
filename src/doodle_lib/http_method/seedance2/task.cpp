#include "doodle_core/metadata/task.h"

#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/seedance2/assets_entity.h>
#include <doodle_core/metadata/seedance2/assets_entity_item.h>
#include <doodle_core/metadata/seedance2/group.h>
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "reg.h"
#include <memory>

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(user_seedance2_task, post) {
  auto l_json = in_handle->get_json();

  auto l_task = std::make_shared<sd2::task>();
  l_json.get_to(*l_task);
  l_task->user_id_ = person_.person_.uuid_id_;
  auto l_sql       = get_sqlite_database();
  co_await l_sql.install(l_task);

  co_return in_handle->make_msg(nlohmann::json{{"id", l_task->uuid_id_}});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(user_seedance2_task, get) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::user_id_) == person_.person_.uuid_id_));
  co_return in_handle->make_msg(l_vec);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_task, get) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec =
      l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::ai_studio_id_) == person_.person_.studio_id_));
  co_return in_handle->make_msg(l_vec);
}
}  // namespace doodle::http::seedance2