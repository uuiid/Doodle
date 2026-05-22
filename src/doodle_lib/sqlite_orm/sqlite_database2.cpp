#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
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

#include "sqlite_orm/detail/dynamic_where.h"
#include "sqlite_select_data.h"
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>
#include <vector>

namespace doodle::sqlite_select {

get_get_entities_and_tasks_select_t get_get_entities_and_tasks_select_t::get(
    const person& in_person, const uuid& in_project_id, const uuid& in_entity_type_id, std::int32_t in_offset,
    std::int32_t in_limit
) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_outsource_select = select(
      &outsource_studio_authorization::entity_id_,
      where(c(&outsource_studio_authorization::studio_id_) == in_person.studio_id_)
  );

  auto l_subscriptions_for_user = l_sql.get_person_subscriptions(in_person, in_project_id, in_entity_type_id);
  auto sequence                 = "sequence"_alias.for_<entity>();

  auto l_rows                   = l_sql.impl_->storage_any_.select(
      columns(object<entity>(true), object<task>(true), &assignees_table::person_id_), from<entity>(),
      left_outer_join<task>(on(c(&entity::uuid_id_) == c(&task::entity_id_))),
      left_outer_join<assignees_table>(on(c(&assignees_table::task_id_) == c(&task::uuid_id_))),
      where(
          c(&entity::entity_type_id_) == in_entity_type_id &&
          ((in_project_id.is_nil() || c(&entity::project_id_) == in_project_id) &&
           (in_person.role_ != person_role_type::outsource || in(&entity::uuid_id_, l_outsource_select)))

      ),
      multi_order_by(order_by(&entity::name_)), limit(in_offset, in_limit)
  );
  auto l_cout = l_sql.impl_->storage_any_.select(
      columns(sequence->*&entity::uuid_id_, count(sequence->*&entity::uuid_id_)), from<entity>(),
      join<sequence>(on(c(&entity::parent_id_) == c(sequence->*&entity::uuid_id_))),
      where(c(&entity::project_id_) == in_project_id), group_by(sequence->*&entity::uuid_id_)
  );
  return get_get_entities_and_tasks_select_t{.entity_and_task_and_person_id_ = l_rows, .sequence_and_cout_ = l_cout};
}

std::vector<entity> get_entity_by_episode_id_and_project_id_and_name(
    const uuid& type_id_, const uuid& in_episode_id, const uuid& in_project_id, const std::string& in_name
) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;

  auto l_dynamic_where = dynamic_where(l_sql.impl_->storage_any_);
  if (!in_name.empty()) l_dynamic_where.push_back(c(&entity::name_) == in_name);
  if (!in_project_id.is_nil()) l_dynamic_where.push_back(c(&entity::project_id_) == in_project_id);
  if (!in_episode_id.is_nil()) l_dynamic_where.push_back(c(&entity::parent_id_) == in_episode_id);
  DOODLE_CHICK(!type_id_.is_nil(), "类别id不可空")
  l_dynamic_where.push_back(c(&entity::entity_type_id_) == type_id_);

  auto l_sq_list = l_sql.impl_->storage_any_.get_all<entity>(where(l_dynamic_where));
  return l_sq_list;
}
bool task_exit_by_entity_id_and_task_type_id(const uuid& in_entity_id, const uuid& in_task_type_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_count = l_sql.impl_->storage_any_.count<task>(
      where(c(&task::entity_id_) == in_entity_id && c(&task::task_type_id_) == in_task_type_id)
  );
  return l_count > 0;
}

std::vector<sd2_select_task_t> get_tasks_and_entity_for_ai_studio(const uuid& in_ai_studio_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<sd2::task>(true), object<entity>(true), object<task>(true), object<entity_shot_extend>(true)),
      from<sd2::task>(), left_outer_join<task>(on(c(&sd2::task::task_id_) == c(&task::uuid_id_))),
      left_outer_join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      left_outer_join<entity_shot_extend>(on(c(&entity_shot_extend::entity_id_) == c(&entity::uuid_id_))),
      where(c(&sd2::task::ai_studio_id_) == in_ai_studio_id)
  );
  std::vector<sd2_select_task_t> l_result{};
  l_result.reserve(l_r.size());
  for (auto&& [l_sd2_task, l_entity, l_task, l_entity_shot_extend] : l_r)
    l_result.emplace_back(l_sd2_task, l_entity, l_task, l_entity_shot_extend);
  return l_result;
}

std::vector<sd2_select_task_t> get_tasks_and_entity_for_person(const uuid& in_person_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.select(
      columns(object<sd2::task>(true), object<entity>(true), object<task>(true), object<entity_shot_extend>(true)),
      from<sd2::task>(), left_outer_join<task>(on(c(&sd2::task::task_id_) == c(&task::uuid_id_))),
      left_outer_join<entity>(on(c(&task::entity_id_) == c(&entity::uuid_id_))),
      left_outer_join<entity_shot_extend>(on(c(&entity_shot_extend::entity_id_) == c(&entity::uuid_id_))),
      where(c(&sd2::task::user_id_) == in_person_id)
  );
  std::vector<sd2_select_task_t> l_result{};
  l_result.reserve(l_r.size());
  for (auto&& [l_sd2_task, l_entity, l_task, l_entity_shot_extend] : l_r)
    l_result.emplace_back(l_sd2_task, l_entity, l_task, l_entity_shot_extend);
  return l_result;
}
std::vector<sd2::task> get_task_for_shot_task_id(const uuid& in_task_id, const uuid& in_ai_studio_id) {
  auto& l_sql = get_sqlite_database();
  using namespace sqlite_orm;
  auto l_r = l_sql.impl_->storage_any_.get_all<sd2::task>(
      where(c(&sd2::task::shot_uuid_id_) == in_task_id && c(&sd2::task::ai_studio_id_) == in_ai_studio_id)
  );
  return l_r;
}
}  // namespace doodle::sqlite_select