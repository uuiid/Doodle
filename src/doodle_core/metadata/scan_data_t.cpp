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

    if (auto l_h = entt::to_entity(*in_registry.storage<project>(), *l_handle.get<project_ptr>().project_);
        l_h != entt::null)
      l_ret.project_ = in_registry.storage<uuid>(detail::project_id)->get(l_h);
    auto& l_a         = l_handle.get<additional_data>();
    l_ret.ue_path_    = l_a.ue_path_.generic_string();
    l_ret.rig_path_   = l_a.rig_path_.generic_string();
    l_ret.solve_path_ = l_a.solve_path_.generic_string();
    l_ret.num_        = l_a.num_;
    l_ret.name_       = l_a.name_;
    l_ret.version_    = l_a.version_;
    l_ret.id_         = to_entity(l_entity);

    g_ctx().get<sqlite_database>()(std::move(l_ret));
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
}
void scan_data_t::on_destroy(entt::registry& in_reg, entt::entity in_entity) {
  g_ctx().get<sqlite_database>().destroy<scan_data_t>(in_reg.storage<entt::id_type>(detail::sql_id).get(in_entity));
}

std::vector<entt::entity> scan_data_t::load_from_sql(
    entt::registry& in_registry, const std::vector<database_t>& in_data
) {
  std::vector<entt::entity> l_create{};
  if (in_data.empty()) return l_create;

  in_registry.create(l_create.begin(), l_create.end());
  g_ctx().erase<scan_data_ctx_t>();
  {
    std::vector<path_uuid> l_vec = in_data | ranges::views::transform([](const database_t& in_db) {
                                     return path_uuid{in_db.ue_uuid_, in_db.rig_uuid_, in_db.solve_uuid_};
                                   }) |
                                   ranges::to_vector;
    in_registry.insert<path_uuid>(l_create.begin(), l_create.end(), l_vec.begin());
  }
  std::map<uuid, entt::entity> l_prj_map{};
  for (auto&& [l_e, l_id] : entt::view<entt::get_t<uuid>>{in_registry.storage<uuid>(detail::project_id)}.each()) {
    l_prj_map.emplace(l_id, l_e);
  }
  {
    std::vector<project_ptr> l_vec = in_data | ranges::views::transform([&](const database_t& in_db) {
                                       return project_ptr{in_registry.try_get<project>(l_prj_map[in_db.project_])};
                                     }) |
                                     ranges::to_vector;
    in_registry.insert<project_ptr>(l_create.begin(), l_create.end(), l_vec.begin());
  }

  std::vector<additional_data> l_vec = in_data | ranges::views::transform([&](const database_t& in_db) {
                                         return additional_data{in_db.ue_path_, in_db.rig_path_, in_db.solve_path_,
                                                                in_db.num_,     in_db.name_,     in_db.version_};
                                       }) |
                                       ranges::to_vector;
  in_registry.insert<additional_data>(l_create.begin(), l_create.end(), l_vec.begin());
  auto& l_ctx    = g_ctx().emplace<scan_data_ctx_t>();
  l_ctx.conn_[0] = in_registry.on_construct<additional_data>().connect<&dependent_uuid>();
  l_ctx.conn_[1] = in_registry.on_update<additional_data>().connect<&dependent_uuid>();
  l_ctx.conn_[2] = in_registry.storage<entt::id_type>(detail::sql_id).on_destroy().connect<&on_destroy>();

  return l_create;
}

}  // namespace doodle