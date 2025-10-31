#include "entity_path.h"
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project.h>

namespace doodle {

class asset_path_base : public entity_path_base {};
class shots_path_base : public entity_path_base {};

class character_path_base : public asset_path_base {};
class prop_path_base : public asset_path_base {};
class ground_path_base : public asset_path_base {};
class binding_path_base : public asset_path_base {};
class simulation_path_base : public asset_path_base {};

class animation_path_base : public shots_path_base {};
class simulation_shots_path_base : public shots_path_base {};

std::shared_ptr<entity_path_base> create_entity_path(const uuid& in_task_id) {
  // TODO 根据 task_id 查询 task_type 和 entity_type，然后创建不同的路径对象

  auto l_task = g_ctx().get<sqlite_database>().get_by_uuid<task>(in_task_id);
  auto l_entity = g_ctx().get<sqlite_database>().get_by_uuid<entity>(l_task.entity_id_);

  

  return nullptr;
}

}  // namespace doodle