//
// Created by TD on 2023/1/13.
//

#pragma once

#include "doodle_core/metadata/work_task.h"
#include <doodle_core/doodle_core_fwd.h>

namespace doodle::database_n {

template <>
struct sql_com<doodle::database, false> {
  registry_ptr reg_;

  void insert(conn_ptr& in_ptr, const std::vector<entt::entity>& in_handle);
  /**
   *
   * @param in_ptr
   * @param in_handle id与之相对的实体
   */
  void select(conn_ptr& in_ptr, std::map<std::int64_t, entt::entity>& in_handle);
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};
}  // namespace doodle::database_n