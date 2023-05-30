#pragma once

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/rules.h>
namespace doodle::database_n {
template <>
struct sql_com<doodle::business::rules> : detail::sql_create_table_base<tables::business_rules> {
  sql_com() = default;
  void create_table(conn_ptr& in_ptr) override;

  void insert_sub(
      conn_ptr& in_ptr, const std::vector<entt::handle>& in_, const std::map<entt::handle, std::int64_t>& in_map
  );
  void insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id);
  void update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id);
  /**
   *
   * @param in_ptr
   * @param in_handle id与之相对的实体
   */
  void select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, const registry_ptr& in_reg);
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};
}  // namespace doodle::database_n