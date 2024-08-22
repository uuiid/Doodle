#pragma once

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/assets_file.h>

#include <entt/entity/fwd.hpp>

namespace doodle::database_n {
template <>
struct sql_com<doodle::assets_file> : detail::sql_create_table_base<tables::assets_file> {
  sql_com() = default;
  void create_table(const sql_connection_ptr& in_ptr) override;
  void insert(const sql_connection_ptr& in_ptr, const std::vector<entt::handle>& in_id);
  void update(const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id);
  void select(const sql_connection_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg);
  void destroy(const sql_connection_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};

}  // namespace doodle::database_n