#pragma once

#include <doodle_core/metadata/export_file_info.h>
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::database_n {
template <>
struct sql_com<doodle::export_file_info> {     
  registry_ptr reg_;

  void create_table(conn_ptr& in_ptr);
  void insert(conn_ptr& in_ptr, const entt::observer& in_observer);
  void update(conn_ptr& in_ptr, const entt::observer& in_observer);
  /**
   *
   * @param in_ptr
   * @param in_handle id与之相对的实体
   */
  void select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle);
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};
}  // namespace doodle::database_n