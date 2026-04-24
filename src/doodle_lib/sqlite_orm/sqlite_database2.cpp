#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/seedance2/assets_entity_item.h"
#include "doodle_core/metadata/seedance2/group.h"
#include "doodle_core/metadata/seedance2/task.h"
#include <doodle_core/metadata/ai_image_metadata.h>
#include <doodle_core/metadata/asset_instance.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/playlist.h>
#include <doodle_core/metadata/preview_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/shot.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/metadata/working_file.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/global_function.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>

#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>

namespace doodle::sqlite_select {

std::vector<ai_studio_and_link_t> ai_studio_and_link_t_get_all() {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ai_studios = l_sql.impl_->storage_any_.select(
      columns(object<ai_studio>(), object<ai_studio_person_role_link>()),
      left_join<ai_studio_person_role_link>(
          on(c(&ai_studio_person_role_link::ai_studio_id_) == c(&ai_studio::uuid_id_))
      )
  );
  std::vector<ai_studio_and_link_t> l_result;
  std::map<uuid, std::size_t> l_map{};
  for (auto&& [ai_studio, link] : l_ai_studios) {
    if (!l_map.contains(ai_studio.uuid_id_)) {
      l_result.emplace_back(ai_studio);
      l_map[ai_studio.uuid_id_] = l_result.size() - 1;
    }
    if (!link.ai_studio_id_.is_nil()) l_result[l_map[ai_studio.uuid_id_]].link_.push_back(link);
  }
  return l_result;
}

std::string get_rig_person_last_name_for_entity(const uuid& in_entity_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;

  auto l_task = l_sql.impl_->storage_any_.select(
      &task::uuid_id_, from<task>(),
      where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == task_type::get_binding_id()), limit(1)
  );
  std::string l_user_abbreviation{};
  if (!l_task.empty()) {
    auto l_user = l_sql.impl_->storage_any_.select(
        &person::last_name_, from<person>(),
        where(
            in(&person::uuid_id_,
               select(&assignees_table::person_id_, where(c(&assignees_table::task_id_) == l_task.front())))
        ),
        limit(1)
    );

    if (!l_user.empty()) return l_user.front();
  }
  return {};
}
uuid get_ai_studio_uuid_for_person(const uuid& in_person_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<ai_studio_person_role_link>(
      where(c(&ai_studio_person_role_link::person_id_) == in_person_id)
  );
  return l_vec.empty() ? uuid{} : l_vec.front().ai_studio_id_;
}
// 获取任务分配的人(连接表)
std::optional<assignees_table> get_task_assignees_for_task_and_person(uuid in_task_id, uuid in_person_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret = l_sql.impl_->storage_any_.get_all<assignees_table>(
      where(c(&assignees_table::task_id_) == in_task_id && c(&assignees_table::person_id_) == in_person_id)
  );
  return !l_ret.empty() ? std::optional{l_ret.front()} : std::optional<assignees_table>{std::nullopt};
}
std::vector<std::int64_t> get_task_assignees_ids_for_task(uuid in_task_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_ret =
      l_sql.impl_->storage_any_.select(&assignees_table::id_, where(c(&assignees_table::task_id_) == in_task_id));
  return l_ret;
}
std::vector<sd2::task> get_sd2_tasks_for_ai_studio(const uuid& in_ai_studio_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::ai_studio_id_) == in_ai_studio_id));
  return l_vec;
}

std::vector<sd2::task> get_sd2_tasks_for_person(const uuid& in_person_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::task>(where(c(&sd2::task::user_id_) == in_person_id));
  return l_vec;
}
std::vector<sd2::assets_group> get_sd2_assets_group_for_ai_studio(const uuid& in_ai_studio_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::assets_group>(
      where(c(&sd2::assets_group::ai_studio_id_) == in_ai_studio_id)
  );
  return l_vec;
}

std::size_t get_sd2_assets_count_for_assets_group(const uuid& in_assets_group_id) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_size = l_sql.impl_->storage_any_.count<sd2::assets_entity>(
      where(c(&sd2::assets_entity::group_id_) == in_assets_group_id)
  );
  return l_size;
}
std::vector<assets_entity_and_item> get_assets_entity_and_item_all_for_person_and_ai_studio(
    const uuid& in_group_id, const uuid& in_ai_studio_id
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_entities = l_sql.impl_->storage_any_.select(
      columns(object<sd2::assets_entity>(), object<sd2::assets_entity_item>()),
      join<sd2::assets_entity_item>(on(c(&sd2::assets_entity_item::parent_id_) == c(&sd2::assets_entity::uuid_id_))),
      where(
          c(&sd2::assets_entity::group_id_) == in_group_id && c(&sd2::assets_entity::ai_studio_id_) == in_ai_studio_id
      )
  );
  std::vector<assets_entity_and_item> l_result{};
  std::map<uuid, std::size_t> l_map{};
  for (auto&& [entity, item] : l_entities) {
    if (!l_map.contains(entity.uuid_id_)) {
      l_result.emplace_back(entity);
      l_map[entity.uuid_id_] = l_result.size() - 1;
    }
    if (!item.uuid_id_.is_nil()) l_result[l_map[entity.uuid_id_]].items_.push_back(item);
  }
  return l_result;
}
std::vector<sd2::assets_entity> search_sd2_assets_entity_for_ai_studio(
    const uuid& in_ai_studio_id, const std::string& keyword
) {
  auto l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_vec = l_sql.impl_->storage_any_.get_all<sd2::assets_entity>(
      where(like(&sd2::assets_entity::name_, keyword) && c(&sd2::assets_entity::ai_studio_id_) == in_ai_studio_id)
  );
  return l_vec;
}
}  // namespace doodle::sqlite_select