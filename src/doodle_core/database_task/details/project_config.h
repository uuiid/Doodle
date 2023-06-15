#pragma once

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/project.h>

#include "entt/entity/fwd.hpp"
#include <cstdint>

namespace doodle::database_n {
template <>
struct sql_com<project_config::base_config> : detail::sql_create_table_base<tables::project_config> {
  sql_com() = default;
  void install_sub(
      conn_ptr& in_ptr, const std::vector<entt::handle>& in_handles, const std::map<entt::handle, std::int64_t>& in_map
  );

  void create_table(conn_ptr& in_ptr) override;
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

template <>
struct sql_ctx<project_config::base_config> : detail::sql_create_table_base<tables::project_config> {
  void install_sub(conn_ptr& in_ptr, const std::int64_t in_id, const project_config::base_config& in_config);
  void create_table(conn_ptr& in_ptr) override;
  /// 插入组件
  void insert(conn_ptr& in_ptr, const project_config::base_config& in_config);

  /// 选择组件
  void select(conn_ptr& in_ptr, project_config::base_config& in_handle);
};

}  // namespace doodle::database_n