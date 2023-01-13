#pragma once

#include "doodle_core/metadata/work_task.h"
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::database_n {
template <>
struct sql_com<doodle::work_task_info, false> {
  struct table_data {};

  void create_table(conn_ptr& in_ptr);
  void insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_handle);
  void update(conn_ptr& in_ptr, const entt::handle& in_handle);
  void select(conn_ptr& in_ptr, entt::handle& in_handle);
  void destroy(conn_ptr& in_ptr, const entt::handle& in_handle);
};
}  // namespace doodle::database_n