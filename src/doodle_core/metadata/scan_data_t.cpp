//
// Created by TD on 24-9-11.
//

#include "scan_data_t.h"

#include "sqlite_orm/sqlite_database.h"
#include <sqlite_orm/sqlite_orm.h>
#include <sqlite_orm/uuid_to_blob.h>
namespace doodle {

void scan_data_t::seed_to_sql(const entt::registry& in_registry, const std::vector<entt::entity>& in_entity) {
  for (auto&& l_entity : in_entity) {
    if (!in_registry.valid(l_entity)) continue;

    entt::const_handle l_handle{in_registry, l_entity};
    database_t l_ret{};
    auto& l_id        = l_handle.get<path_uuid>();
    l_ret.ue_uuid_    = l_id.ue_uuid_;
    l_ret.rig_uuid_   = l_id.rig_uuid_;
    l_ret.solve_uuid_ = l_id.solve_uuid_;

    auto& l_a         = l_handle.get<additional_data>();
    l_ret.ue_path_    = l_a.ue_path_.generic_string();
    l_ret.rig_path_   = l_a.rig_path_.generic_string();
    l_ret.solve_path_ = l_a.solve_path_.generic_string();
    l_ret.num_        = l_a.num_;
    l_ret.name_       = l_a.name_;
    l_ret.version_    = l_a.version_;
    l_ret.id_         = to_entity(l_entity);

    // g_ctx().get<sqlite_database>()(std::move(l_ret));
  }
}

void scan_data_t::dependent_uuid(entt::registry& in_reg, entt::entity in_entity) {
  auto& l_data = in_reg.get<additional_data>(in_entity);

  auto& l_uuid = in_reg.get_or_emplace<path_uuid>(in_entity);
  try {
    l_uuid.rig_uuid_   = FSys::software_flag_file(l_data.rig_path_);
    l_uuid.ue_uuid_    = FSys::software_flag_file(l_data.ue_path_);
    l_uuid.solve_uuid_ = FSys::software_flag_file(l_data.solve_path_);

  } catch (...) {
    default_logger_raw()->error(boost::current_exception_diagnostic_information());
  }
  if (!in_reg.all_of<project_ptr>(in_entity)) in_reg.emplace<project_ptr>(in_entity);
}
void scan_data_t::on_destroy(entt::registry& in_reg, entt::entity in_entity) {
  // g_ctx().get<sqlite_database>().destroy<scan_data_t>(in_reg.storage<entt::id_type>(detail::sql_id).get(in_entity));
}

std::vector<entt::entity> scan_data_t::load_from_sql(
    entt::registry& in_registry, const std::vector<database_t>& in_data
) {
  return {};
}

}  // namespace doodle