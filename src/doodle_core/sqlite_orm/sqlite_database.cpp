//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/kitsu/assets_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/metadata_descriptor.h>
#include <doodle_core/metadata/preview_background_file.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/macro.h>
#include <doodle_core/sqlite_orm/detail/nlohmann_json.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_duration.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_time_point.h>
#include <doodle_core/sqlite_orm/detail/std_chrono_zoned_time.h>
#include <doodle_core/sqlite_orm/detail/std_filesystem_path_orm.h>
#include <doodle_core/sqlite_orm/detail/uuid_to_blob.h>

#include "metadata/attendance.h"
#include "metadata/status_automation.h"
#include <sqlite_orm/sqlite_orm.h>

namespace doodle {

void sqlite_database::load(const FSys::path& in_path) { impl_ = std::make_shared<sqlite_database_impl>(in_path); }

std::vector<project_helper::database_t> sqlite_database::find_project_by_name(const std::string& in_name) {
  return impl_->find_project_by_name(in_name);
}
std::vector<server_task_info> sqlite_database::get_server_task_info(const uuid& in_computer_id) {
  return impl_->get_server_task_info(in_computer_id);
}

std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  return impl_->get_attendance(in_ref_id, in_data);
}
std::vector<attendance_helper::database_t> sqlite_database::get_attendance(
    const std::int64_t& in_ref_id, const std::vector<chrono::local_days>& in_data
) {
  return impl_->get_attendance(in_ref_id, in_data);
}
std::vector<work_xlsx_task_info_helper::database_t> sqlite_database::get_work_xlsx_task_info(
    const std::int64_t& in_ref_id, const chrono::local_days& in_data
) {
  return impl_->get_work_xlsx_task_info(in_ref_id, in_data);
}
std::vector<server_task_info> sqlite_database::get_server_task_info_by_user(const uuid& in_user_id) {
  return impl_->get_server_task_info_by_user(in_user_id);
}
std::vector<server_task_info> sqlite_database::get_server_task_info_by_type(const server_task_info_type& in_user_id) {
  return impl_->get_server_task_info_by_type(in_user_id);
}

DOODLE_GET_BY_PARENT_ID_SQL(assets_file_helper::database_t);
DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(project_helper::database_t)
DOODLE_UUID_TO_ID(user_helper::database_t)
DOODLE_UUID_TO_ID(metadata::kitsu::task_type_t)
DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)
DOODLE_UUID_TO_ID(computer)

DOODLE_GET_BY_UUID_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_GET_BY_UUID_SQL(user_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_BY_UUID_SQL(assets_file_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)
DOODLE_GET_BY_UUID_SQL(project_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::assets_type_t)
DOODLE_GET_BY_UUID_SQL(computer)
DOODLE_GET_BY_UUID_SQL(server_task_info)

DOODLE_GET_ALL_SQL(project_helper::database_t)
DOODLE_GET_ALL_SQL(user_helper::database_t)
DOODLE_GET_ALL_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_ALL_SQL(assets_file_helper::database_t)
DOODLE_GET_ALL_SQL(metadata::kitsu::assets_type_t)
DOODLE_GET_ALL_SQL(assets_helper::database_t)
DOODLE_GET_ALL_SQL(computer)
DOODLE_GET_ALL_SQL(server_task_info)
DOODLE_GET_ALL_SQL(project_status)
DOODLE_GET_ALL_SQL(task_status)
DOODLE_GET_ALL_SQL(task_type)

DOODLE_INSTALL_SQL(project_helper::database_t)
DOODLE_INSTALL_SQL(user_helper::database_t)
DOODLE_INSTALL_SQL(metadata::kitsu::task_type_t)
DOODLE_INSTALL_SQL(assets_file_helper::database_t)
DOODLE_INSTALL_SQL(assets_helper::database_t)
DOODLE_INSTALL_SQL(computer)
DOODLE_INSTALL_SQL(server_task_info)
DOODLE_INSTALL_SQL(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_SQL(project)
DOODLE_INSTALL_SQL(project_status)
DOODLE_INSTALL_SQL(task_status)
DOODLE_INSTALL_SQL(task_type)

DOODLE_INSTALL_RANGE(project_helper::database_t)
DOODLE_INSTALL_RANGE(attendance_helper::database_t)
DOODLE_INSTALL_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_INSTALL_RANGE(metadata::kitsu::task_type_t)
DOODLE_INSTALL_RANGE(assets_helper::database_t)
DOODLE_INSTALL_RANGE(assets_file_helper::database_t)
DOODLE_INSTALL_RANGE(metadata::kitsu::assets_type_t)
DOODLE_INSTALL_RANGE(computer)
DOODLE_INSTALL_RANGE(task_status)
DOODLE_INSTALL_RANGE(task_type)

DOODLE_REMOVE_RANGE(attendance_helper::database_t)
DOODLE_REMOVE_RANGE(work_xlsx_task_info_helper::database_t)
DOODLE_REMOVE_RANGE(metadata::kitsu::task_type_t)
DOODLE_REMOVE_RANGE(assets_file_helper::database_t)
DOODLE_REMOVE_RANGE(assets_helper::database_t)
DOODLE_REMOVE_RANGE(computer)

DOODLE_REMOVE_BY_UUID(assets_helper::database_t)
DOODLE_REMOVE_BY_UUID(assets_file_helper::database_t)
DOODLE_REMOVE_BY_UUID(computer)
DOODLE_REMOVE_BY_UUID(server_task_info)

}  // namespace doodle