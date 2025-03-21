//
// Created by TD on 24-9-12.
//

#include "sqlite_database.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/kitsu/assets_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/server_task_info.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_xlsx_task_info.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>

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

std::int32_t sqlite_database::get_notification_count(const uuid& in_user_id) {
  return impl_->get_notification_count(in_user_id);
}

std::vector<project_with_extra_data> sqlite_database::get_project_for_user(const person& in_user) {
  std::vector<project_with_extra_data> l_projects{};
  if (in_user.role_ == person_role_type::admin) {
    auto l_t   = impl_->get_all<project>();
    l_projects = l_t | ranges::views::transform([](const project& in) { return project_with_extra_data{in}; }) |
                 ranges::to_vector;
  } else {
    auto l_project_person_link = impl_->storage_any_.get_all<project_person_link>(
        sqlite_orm::where(sqlite_orm::c(&project_person_link::person_id_) == in_user.uuid_id_)
    );
    auto l_project_ids = l_project_person_link |
                         ranges::views::transform([](const project_person_link& in) { return in.project_id_; }) |
                         ranges::to_vector;
    auto l_t =
        impl_->storage_any_.get_all<project>(sqlite_orm::where(sqlite_orm::in(&project::uuid_id_, l_project_ids)));
    l_projects = l_t | ranges::views::transform([](const project& in) { return project_with_extra_data{in}; }) |
                 ranges::to_vector;
  }

  for (auto&& i : l_projects) {
    auto l_project_person_link = impl_->storage_any_.get_all<project_person_link>(
        sqlite_orm::where(sqlite_orm::c(&project_person_link::project_id_) == i.uuid_id_)
    );
    i.team_ = l_project_person_link |
              ranges::views::transform([](const project_person_link& in) { return in.person_id_; }) | ranges::to_vector;
    auto l_asset_type_link = impl_->storage_any_.get_all<project_asset_type_link>(
        sqlite_orm::where(sqlite_orm::c(&project_asset_type_link::project_id_) == i.uuid_id_)
    );
    i.asset_types_ = l_asset_type_link |
                     ranges::views::transform([](const project_asset_type_link& in) { return in.asset_type_id_; }) |
                     ranges::to_vector;
    auto l_task_status_link = impl_->storage_any_.get_all<project_task_status_link>(
        sqlite_orm::where(sqlite_orm::c(&project_task_status_link::project_id_) == i.uuid_id_)
    );
    i.task_statuses_ = l_task_status_link |
                       ranges::views::transform([](const project_task_status_link& in) { return in.task_status_id_; }) |
                       ranges::to_vector;
    auto l_task_type_link = impl_->storage_any_.get_all<project_task_type_link>(
        sqlite_orm::where(sqlite_orm::c(&project_task_type_link::project_id_) == i.uuid_id_)
    );
    i.task_types_ = l_task_type_link |
                    ranges::views::transform([](const project_task_type_link& in) { return in.task_type_id_; }) |
                    ranges::to_vector;
    auto l_status_automations = impl_->storage_any_.get_all<project_status_automation_link>(
        sqlite_orm::where(sqlite_orm::c(&project_status_automation_link::project_id_) == i.uuid_id_)
    );
    i.status_automations_ =
        l_status_automations |
        ranges::views::transform([](const project_status_automation_link& in) { return in.status_automation_id_; }) |
        ranges::to_vector;
    auto l_preview_background_files = impl_->storage_any_.get_all<project_preview_background_file_link>(
        sqlite_orm::where(sqlite_orm::c(&project_preview_background_file_link::project_id_) == i.uuid_id_)
    );
    i.preview_background_files_ = l_preview_background_files |
                                  ranges::views::transform([](const project_preview_background_file_link& in) {
                                    return in.preview_background_file_id_;
                                  }) |
                                  ranges::to_vector;

    auto l_descriptors = impl_->storage_any_.get_all<metadata_descriptor>(
        sqlite_orm::left_join<metadata_descriptor_department_link>(sqlite_orm::on(
            sqlite_orm::c(&metadata_descriptor::uuid_id_) ==
            sqlite_orm::c(&metadata_descriptor_department_link::metadata_descriptor_uuid_)
        )),
        sqlite_orm::where(
            sqlite_orm::in(&metadata_descriptor_department_link::department_uuid_, in_user.departments_) ||
            sqlite_orm::is_null(&metadata_descriptor_department_link::department_uuid_)
        )
    );
    i.descriptors_         = l_descriptors;
    i.task_types_priority_ = l_task_type_link;
    i.task_statuses_link_  = l_task_status_link;
  }
  return {};
}

DOODLE_GET_BY_PARENT_ID_SQL(assets_file_helper::database_t);
DOODLE_GET_BY_PARENT_ID_SQL(assets_helper::database_t);

DOODLE_UUID_TO_ID(project_helper::database_t)
DOODLE_UUID_TO_ID(user_helper::database_t)
DOODLE_UUID_TO_ID(metadata::kitsu::task_type_t)
DOODLE_UUID_TO_ID(assets_file_helper::database_t)
DOODLE_UUID_TO_ID(assets_helper::database_t)
DOODLE_UUID_TO_ID(computer)
DOODLE_UUID_TO_ID(person)
DOODLE_UUID_TO_ID(attendance_helper::database_t)

DOODLE_ID_TO_UUID(project_helper::database_t)
DOODLE_ID_TO_UUID(user_helper::database_t)
DOODLE_ID_TO_UUID(metadata::kitsu::task_type_t)
DOODLE_ID_TO_UUID(assets_file_helper::database_t)
DOODLE_ID_TO_UUID(assets_helper::database_t)
DOODLE_ID_TO_UUID(computer)
DOODLE_ID_TO_UUID(attendance_helper::database_t)

template <>
work_xlsx_task_info_helper::database_t sqlite_database::get_by_uuid<work_xlsx_task_info_helper::database_t>(
    const uuid& in_uuid
) {
  auto l_list     = impl_->get_by_uuid<work_xlsx_task_info_helper::database_t>(in_uuid);
  l_list.user_id_ = impl_->id_to_uuid<user_helper::database_t>(l_list.user_ref_);
  return l_list;
}
template <>
person sqlite_database::get_by_uuid<person>(const uuid& in_uuid) {
  auto l_p    = impl_->get_by_uuid<person>(in_uuid);
  auto l_list = impl_->storage_any_.select(
      sqlite_orm::columns(&person_department_link::department_id_),
      sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == in_uuid)
  );
  l_p.departments_ = l_list | ranges::views::transform([](const std::tuple<uuid>& in) { return std::get<uuid>(in); }) |
                     ranges::to_vector;
  return l_p;
}

DOODLE_GET_BY_UUID_SQL(user_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::task_type_t)
DOODLE_GET_BY_UUID_SQL(assets_file_helper::database_t)
DOODLE_GET_BY_UUID_SQL(assets_helper::database_t)
DOODLE_GET_BY_UUID_SQL(project_helper::database_t)
DOODLE_GET_BY_UUID_SQL(metadata::kitsu::assets_type_t)
DOODLE_GET_BY_UUID_SQL(computer)
DOODLE_GET_BY_UUID_SQL(server_task_info)
DOODLE_GET_BY_UUID_SQL(attendance_helper::database_t)

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
DOODLE_GET_ALL_SQL(department)
DOODLE_GET_ALL_SQL(studio)
DOODLE_GET_ALL_SQL(status_automation)
DOODLE_GET_ALL_SQL(organisation)
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
    auto l_list_dep = impl_->storage_any_.select(
        sqlite_orm::columns(&person_department_link::department_id_),
        sqlite_orm::where(sqlite_orm::c(&person_department_link::person_id_) == i.uuid_id_)
    );
    i.departments_ = l_list_dep |
                     ranges::views::transform([](const std::tuple<uuid>& in) { return std::get<uuid>(in); }) |
                     ranges::to_vector;
  }

  return l_list;
}

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
DOODLE_INSTALL_SQL(asset_type)
DOODLE_INSTALL_SQL(task_type_asset_type_link)
DOODLE_INSTALL_SQL(department)
DOODLE_INSTALL_SQL(person)
DOODLE_INSTALL_SQL(attendance_helper::database_t)

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
DOODLE_INSTALL_RANGE(asset_type)

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
DOODLE_REMOVE_BY_UUID(attendance_helper::database_t)

}  // namespace doodle