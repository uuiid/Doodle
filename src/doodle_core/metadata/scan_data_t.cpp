//
// Created by TD on 24-9-11.
//

#include "scan_data_t.h"

#include <sqlite_orm/sqlite_orm.h>
namespace doodle {

void scan_data_t::ue_path(const FSys::path& in_path) {
  BOOST_ASSERT(handle_);
  auto l_uuid = FSys::software_flag_file(in_path);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().ue_path_ = in_path;
  } else {
    handle_.emplace<additional_data>().ue_path_ = in_path;
  }
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::ue_path_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = l_uuid;
  } else {
    l_s.emplace(handle_, l_uuid);
  }
}

void scan_data_t::rig_path(const FSys::path& in_path) {
  BOOST_ASSERT(handle_);
  auto l_uuid = FSys::software_flag_file(in_path);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().rig_path_ = in_path;
  } else {
    handle_.emplace<additional_data>().rig_path_ = in_path;
  }
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::rig_path_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = l_uuid;
  } else {
    l_s.emplace(handle_, l_uuid);
  }
}

void scan_data_t::solve_path(const FSys::path& in_path) {
  BOOST_ASSERT(handle_);
  auto l_uuid = FSys::software_flag_file(in_path);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().solve_path_ = in_path;
  } else {
    handle_.emplace<additional_data>().solve_path_ = in_path;
  }
  if (auto& l_s = handle_.registry()->storage<uuid>(detail::solve_path_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = l_uuid;
  } else {
    l_s.emplace(handle_, l_uuid);
  }
}

void scan_data_t::project(entt::entity in_project) {
  BOOST_ASSERT(handle_);
  if (auto& l_s = handle_.registry()->storage<entt::id_type>(detail::project_ref_id); l_s.contains(handle_)) {
    l_s.patch(handle_) = entt::to_integral(in_project);
  } else {
    l_s.emplace(handle_, entt::to_integral(in_project));
  }
}

void scan_data_t::num_str(const std::string& in_num) {
  BOOST_ASSERT(handle_);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().num_ = in_num;
  } else {
    handle_.emplace<additional_data>().num_ = in_num;
  }
}
void scan_data_t::name(const std::string& in_name) {
  BOOST_ASSERT(handle_);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().name_ = in_name;
  } else {
    handle_.emplace<additional_data>().name_ = in_name;
  }
}
void scan_data_t::version(const std::string& in_version) {
  BOOST_ASSERT(handle_);
  if (handle_.any_of<additional_data>()) {
    handle_.patch<additional_data>().version_ = in_version;
  } else {
    handle_.emplace<additional_data>().version_ = in_version;
  }
}
namespace {
void scan_data_save() {
  using namespace sqlite_orm;
  auto l_storage = make_storage(
      "", make_table(
              "scan_data", make_column("id", &scan_data_t::database_t::id_, primary_key()),
              make_column("ue_uuid", &scan_data_t::database_t::ue_uuid_),
              make_column("rig_uuid", &scan_data_t::database_t::rig_uuid_),
              make_column("solve_uuid", &scan_data_t::database_t::solve_uuid_),

              make_column("ue_path", &scan_data_t::database_t::ue_path_),
              make_column("rig_path", &scan_data_t::database_t::rig_path_),
              make_column("solve_path", &scan_data_t::database_t::solve_path_),

              make_column("project", &scan_data_t::database_t::project_),
              make_column("num", &scan_data_t::database_t::num_), make_column("name", &scan_data_t::database_t::name_),
              make_column("version", &scan_data_t::database_t::version_)
          )
  );
  l_storage.sync_schema(true);
}
}  // namespace

}  // namespace doodle