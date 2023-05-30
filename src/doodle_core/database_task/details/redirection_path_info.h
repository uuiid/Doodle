#pragma once

#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/redirection_path_info.h>

#include "core/core_help_impl.h"
#include <cstdint>

namespace doodle::database_n {
template <>
struct sql_com<doodle::redirection_path_info> : detail::sql_create_table_base<tables::redirection_path_info> {
  sql_com() = default;
  void create_table(conn_ptr& in_ptr);
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