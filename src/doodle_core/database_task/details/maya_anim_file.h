//
// Created by TD on 2023/12/18.
//
#pragma once

#include <doodle_core/core/core_help_impl.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/maya_anim_file.h>

namespace doodle::database_n {
template <>
struct sql_com<doodle::maya_anim_file> : detail::sql_create_table_base<tables::maya_anim_file> {
  void install_sub(
      conn_ptr& in_ptr, const std::vector<entt::handle>& in_handles, const std::map<entt::handle, std::int64_t>& in_map
  );

  sql_com() = default;
  void create_table(conn_ptr& in_ptr) override;
  void insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id);
  void update(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_id);
  void select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle, entt::registry& in_reg);
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};
}  // namespace doodle::database_n