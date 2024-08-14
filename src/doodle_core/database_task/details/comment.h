#pragma once

#include <doodle_core/core/core_help_impl.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/comment.h>

#include <cstdint>

namespace doodle::database_n {
template <>
struct sql_com<doodle::comment> : detail::sql_create_table_base<tables::comment> {
  sql_com() = default;
  void insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id);
  void update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id);
  void select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg);
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};
}  // namespace doodle::database_n