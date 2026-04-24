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
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/logger/logger.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>
#include <doodle_lib/sqlite_orm/sqlite_upgrade.h>
#include <doodle_lib/sqlite_orm/tokenizer/sqlite_jieba.h>

#include "sqlite_database.h"
#include "tokenizer/sqlite_jieba.h"
#include <cstddef>
#include <optional>
#include <spdlog/spdlog.h>
#include <sqlite3.h>
#include <sqlite_orm/sqlite_orm.h>


namespace doodle {
DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)
DOODLE_UUID_TO_ID(person)
DOODLE_UUID_TO_ID(entity)
DOODLE_UUID_TO_ID(attendance_helper::database_t)
DOODLE_UUID_TO_ID(asset_type)
DOODLE_UUID_TO_ID(ai_image_metadata)
DOODLE_UUID_TO_ID(project)
DOODLE_UUID_TO_ID(task)
DOODLE_UUID_TO_ID(ai_studio)

DOODLE_ID_TO_UUID(assets_file_helper::database_t)
DOODLE_ID_TO_UUID(assets_helper::database_t)
DOODLE_ID_TO_UUID(attendance_helper::database_t)
DOODLE_ID_TO_UUID(notification)
DOODLE_ID_TO_UUID(ai_studio)

template <>
person sqlite_database::get_by_uuid<person>(const uuid& in_uuid) {
  auto l_p         = impl_->get_by_uuid<person>(in_uuid);
  l_p.departments_ = impl_->storage_any_.select(
      &person_department_link::department_id_,
      sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == in_uuid)
  );
  return l_p;
}

DOODLE_GET_BY_UUID_SQL(preview_file)
DOODLE_GET_BY_UUID_SQL(attachment_file)
template <>
assets_file_helper::database_t sqlite_database::get_by_uuid<assets_file_helper::database_t>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_data          = impl_->get_by_uuid<assets_file_helper::database_t>(in_uuid);
  l_data.uuid_parents_ = impl_->storage_any_.select(
      &assets_file_helper::link_parent_t::assets_type_uuid_,
      where(c(&assets_file_helper::link_parent_t::assets_uuid_) == in_uuid)
  );
  return l_data;
}
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)
DOODLE_GET_BY_UUID_SQL(server_task_info)
DOODLE_GET_BY_UUID_SQL(project_status)
DOODLE_GET_BY_UUID_SQL(task_type)
DOODLE_GET_BY_UUID_SQL(asset_type)
DOODLE_GET_BY_UUID_SQL(department)
DOODLE_GET_BY_UUID_SQL(task_status)
DOODLE_GET_BY_UUID_SQL(notification)
DOODLE_GET_BY_UUID_SQL(comment)
DOODLE_GET_BY_UUID_SQL(playlist)
DOODLE_GET_BY_UUID_SQL(ai_image_metadata)
DOODLE_GET_BY_UUID_SQL(organisation)
DOODLE_GET_BY_UUID_SQL(playlist_shot)
DOODLE_GET_BY_UUID_SQL(computer)
DOODLE_GET_BY_UUID_SQL(studio)
DOODLE_GET_BY_UUID_SQL(ai_studio)
template <>
task sqlite_database::get_by_uuid<task>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_ret = impl_->get_by_uuid<task>(in_uuid);
  l_ret.assignees_ =
      impl_->storage_any_.select(&assignees_table::person_id_, where(c(&assignees_table::task_id_) == in_uuid));
  return l_ret;
}
DOODLE_GET_BY_UUID_SQL(work_xlsx_task_info_helper::database_t)
template <>
entity sqlite_database::get_by_uuid<entity>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_ret                  = impl_->get_by_uuid<entity>(in_uuid);
  l_ret.entities_out          = impl_->storage_any_.select(&entity::uuid_id_, where(c(&entity::parent_id_) == in_uuid));
  l_ret.entity_concept_links_ = impl_->storage_any_.select(
      &entity_concept_link::entity_id_, where(c(&entity_concept_link::entity_id_) == in_uuid)
  );
  return l_ret;
  // l_ret.instance_casting_ = impl_->storage_any_.select(
  //     &asset_instance::instance_id_,
  //     where(c(&asset_instance::entity_id_) == in_uuid)
  // )
}
template <>
project sqlite_database::get_by_uuid<project>(const uuid& in_uuid) {
  using namespace sqlite_orm;
  auto l_prj  = impl_->get_by_uuid<project>(in_uuid);
  l_prj.team_ = impl_->storage_any_.select(
      &project_person_link::person_id_, where(c(&project_person_link::project_id_) == in_uuid)
  );
  l_prj.asset_types_ = impl_->storage_any_.select(
      &project_asset_type_link::asset_type_id_, where(c(&project_asset_type_link::project_id_) == in_uuid)
  );
  l_prj.task_statuses_ = impl_->storage_any_.select(
      &project_task_status_link::uuid_id_, where(c(&project_task_status_link::project_id_) == in_uuid)
  );
  l_prj.task_types_ = impl_->storage_any_.select(
      &project_task_type_link::task_type_id_, where(c(&project_task_type_link::project_id_) == in_uuid)
  );
  l_prj.status_automations_ = impl_->storage_any_.select(
      &project_status_automation_link::status_automation_id_,
      where(c(&project_status_automation_link::project_id_) == in_uuid)
  );
  l_prj.preview_background_files_ = impl_->storage_any_.select(
      &project_preview_background_file_link::preview_background_file_id_,
      where(c(&project_preview_background_file_link::project_id_) == in_uuid)
  );
  return l_prj;
}
DOODLE_GET_BY_UUID_SQL(attendance_helper::database_t)
DOODLE_GET_BY_UUID_SQL(outsource_studio_authorization)
DOODLE_GET_BY_UUID_SQL(seedance2::assets_entity)
DOODLE_GET_BY_UUID_SQL(seedance2::assets_group)
DOODLE_GET_BY_UUID_SQL(seedance2::task)
DOODLE_GET_BY_UUID_SQL(seedance2::assets_entity_item)
template <>
std::vector<assets_file_helper::database_t> sqlite_database::get_all() {
  auto l_list = impl_->get_all<assets_file_helper::database_t>();
  auto l_map  = l_list |
               ranges::views::transform(
                   [](assets_file_helper::database_t& i) -> std::pair<uuid, assets_file_helper::database_t*> {
                     return {i.uuid_id_, &i};
                   }
               ) |
               ranges::to<std::map<uuid, assets_file_helper::database_t*>>();
  auto l_link = impl_->get_all<assets_file_helper::link_parent_t>();
  for (auto&& i : l_link) {
    if (l_map.contains(i.assets_uuid_)) l_map.at(i.assets_uuid_)->uuid_parents_.emplace_back(i.assets_type_uuid_);
  }
  return l_list;
}
DOODLE_GET_ALL_SQL(assets_helper::database_t)
DOODLE_GET_ALL_SQL(server_task_info)
DOODLE_GET_ALL_SQL(project_status)
DOODLE_GET_ALL_SQL(task_status)
DOODLE_GET_ALL_SQL(task_type)
DOODLE_GET_ALL_SQL(department)
DOODLE_GET_ALL_SQL(studio)
DOODLE_GET_ALL_SQL(status_automation)
DOODLE_GET_ALL_SQL(organisation)
DOODLE_GET_ALL_SQL(ai_image_metadata)
DOODLE_GET_ALL_SQL(seedance2::assets_entity)
DOODLE_GET_ALL_SQL(seedance2::assets_group)
DOODLE_GET_ALL_SQL(seedance2::task)
DOODLE_GET_ALL_SQL(seedance2::assets_entity_item)
DOODLE_GET_ALL_SQL(project)
DOODLE_GET_ALL_SQL(ai_studio_person_role_link)
DOODLE_GET_ALL_SQL(ai_studio)
DOODLE_GET_ALL_SQL(outsource_studio_authorization)
DOODLE_GET_ALL_SQL(computer)
template <>
std::vector<asset_type> sqlite_database::get_all() {
  auto l_list = impl_->get_all<asset_type>();
  auto l_map =
      l_list |
      ranges::views::transform([](asset_type& i) -> std::pair<uuid, asset_type*> { return {i.uuid_id_, &i}; }) |
      ranges::to<std::map<uuid, asset_type*>>();
  auto l_link = impl_->get_all<task_type_asset_type_link>();
  for (auto&& i : impl_->get_all<task_type_asset_type_link>()) {
    if (l_map.contains(i.asset_type_id_)) l_map.at(i.asset_type_id_)->task_types_.emplace_back(i.task_type_id_);
  }
  return l_list;
}
template <>
std::vector<person> sqlite_database::get_all() {
  auto l_list = impl_->get_all<person>();
  for (auto&& i : l_list) {
    i.departments_ = impl_->storage_any_.select(
        &person_department_link::department_id_,
        sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == i.uuid_id_)
    );
  }
  return l_list;
}

DOODLE_INSTALL_SQL(assets_file_helper::database_t)
DOODLE_INSTALL_SQL(assets_helper::database_t)
DOODLE_INSTALL_SQL(attachment_file)
DOODLE_INSTALL_SQL(comment_mentions)
DOODLE_INSTALL_SQL(comment_department_mentions)
DOODLE_INSTALL_SQL(notification)
DOODLE_INSTALL_SQL(server_task_info)
DOODLE_INSTALL_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_SQL(project)
DOODLE_INSTALL_SQL(ai_studio)
DOODLE_INSTALL_SQL(project_status)
DOODLE_INSTALL_SQL(task_status)
DOODLE_INSTALL_SQL(task_type)
DOODLE_INSTALL_SQL(asset_type)
DOODLE_INSTALL_SQL(task_type_asset_type_link)
DOODLE_INSTALL_SQL(project_task_type_link)
DOODLE_INSTALL_SQL(project_task_status_link)
DOODLE_INSTALL_SQL(ai_studio_person_role_link)
DOODLE_INSTALL_SQL(department)
DOODLE_INSTALL_SQL(entity_asset_extend)
DOODLE_INSTALL_SQL(person)
DOODLE_INSTALL_SQL(assignees_table)
DOODLE_INSTALL_SQL(preview_file)
DOODLE_INSTALL_SQL(comment_preview_link)
DOODLE_INSTALL_SQL(task)
DOODLE_INSTALL_SQL(comment)
DOODLE_INSTALL_SQL(project_asset_type_link)
DOODLE_INSTALL_SQL(playlist_shot)
DOODLE_INSTALL_SQL(entity)
DOODLE_INSTALL_SQL(project_person_link)
DOODLE_INSTALL_SQL(comment_acknoledgments)
DOODLE_INSTALL_SQL(attendance_helper::database_t)
DOODLE_INSTALL_SQL(assets_file_helper::link_parent_t)
DOODLE_INSTALL_SQL(playlist)
DOODLE_INSTALL_SQL(ai_image_metadata)
DOODLE_INSTALL_SQL(project_status_automation_link)
DOODLE_INSTALL_SQL(entity_shot_extend)
DOODLE_INSTALL_SQL(status_automation)
DOODLE_INSTALL_SQL(studio)
DOODLE_INSTALL_SQL(computer)
DOODLE_INSTALL_SQL(outsource_studio_authorization)
DOODLE_INSTALL_SQL(seedance2::assets_entity)
DOODLE_INSTALL_SQL(seedance2::assets_group)
DOODLE_INSTALL_SQL(seedance2::task)
DOODLE_INSTALL_SQL(seedance2::assets_entity_item)

DOODLE_INSTALL_RANGE(attendance_helper::database_t)
DOODLE_INSTALL_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_RANGE(assets_helper::database_t)
DOODLE_INSTALL_RANGE(assets_file_helper::database_t)
DOODLE_INSTALL_RANGE(comment_mentions)
DOODLE_INSTALL_RANGE(comment_department_mentions)
DOODLE_INSTALL_RANGE(notification)
DOODLE_INSTALL_RANGE(task_status)
DOODLE_INSTALL_RANGE(task_type)
DOODLE_INSTALL_RANGE(entity)
DOODLE_INSTALL_RANGE(ai_studio)
DOODLE_INSTALL_RANGE(asset_type)
DOODLE_INSTALL_RANGE(assets_file_helper::link_parent_t)
DOODLE_INSTALL_RANGE(task)
DOODLE_INSTALL_RANGE(person_department_link)
DOODLE_INSTALL_RANGE(entity_link)
DOODLE_INSTALL_RANGE(playlist_shot)
DOODLE_INSTALL_RANGE(task_type_asset_type_link)
DOODLE_INSTALL_RANGE(assignees_table)
DOODLE_INSTALL_RANGE(entity_shot_extend)
DOODLE_INSTALL_RANGE(seedance2::assets_entity)
DOODLE_INSTALL_RANGE(seedance2::assets_group)
DOODLE_INSTALL_RANGE(seedance2::task)
DOODLE_INSTALL_RANGE(seedance2::assets_entity_item)

DOODLE_REMOVE_BY_ID(attendance_helper::database_t)
DOODLE_REMOVE_BY_ID(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_BY_ID(assets_file_helper::database_t)
DOODLE_REMOVE_BY_ID(assets_helper::database_t)
DOODLE_REMOVE_BY_ID(assignees_table)
DOODLE_REMOVE_BY_ID(comment)
DOODLE_REMOVE_BY_ID(assets_file_helper::link_parent_t)
DOODLE_REMOVE_BY_ID(comment_acknoledgments)
DOODLE_REMOVE_BY_ID(project_person_link)
DOODLE_REMOVE_BY_ID(entity_link)
DOODLE_REMOVE_BY_ID(playlist)
DOODLE_REMOVE_BY_ID(ai_studio_person_role_link)
DOODLE_REMOVE_BY_ID(computer)
DOODLE_REMOVE_BY_ID(ai_studio)
DOODLE_REMOVE_BY_ID(playlist_shot)
DOODLE_REMOVE_BY_ID(entity_shot_extend)
DOODLE_REMOVE_BY_ID(project_status_automation_link)
DOODLE_REMOVE_BY_ID(outsource_studio_authorization)
DOODLE_REMOVE_BY_ID(studio)
DOODLE_REMOVE_BY_ID(task)
DOODLE_REMOVE_BY_ID(seedance2::assets_entity)
DOODLE_REMOVE_BY_ID(seedance2::assets_group)
DOODLE_REMOVE_BY_ID(seedance2::task)
DOODLE_REMOVE_BY_ID(seedance2::assets_entity_item)

DOODLE_REMOVE_BY_UUID(attendance_helper::database_t)
DOODLE_REMOVE_BY_UUID(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_file_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_helper::database_t)
DOODLE_REMOVE_BY_UUID(playlist)
DOODLE_REMOVE_BY_UUID(entity_link)
DOODLE_REMOVE_BY_UUID(ai_image_metadata)
DOODLE_REMOVE_BY_UUID(playlist_shot)
DOODLE_REMOVE_BY_UUID(studio)
DOODLE_REMOVE_BY_UUID(computer)
DOODLE_REMOVE_BY_UUID(outsource_studio_authorization)
DOODLE_REMOVE_BY_UUID(task)
DOODLE_REMOVE_BY_UUID(entity)
DOODLE_REMOVE_BY_UUID(comment)
DOODLE_REMOVE_BY_UUID(server_task_info)
DOODLE_REMOVE_BY_UUID(ai_studio)
DOODLE_REMOVE_BY_UUID(project_task_status_link)
DOODLE_REMOVE_BY_UUID(seedance2::assets_entity)
DOODLE_REMOVE_BY_UUID(seedance2::assets_group)
DOODLE_REMOVE_BY_UUID(seedance2::task)
DOODLE_REMOVE_BY_UUID(seedance2::assets_entity_item)
}  // namespace doodle