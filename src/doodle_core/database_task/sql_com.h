#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <cstdint>
#include <entt/entity/fwd.hpp>

namespace doodle::database_n {

/**
 * @brief 这是一个模板类, 用来特化数据库类型的
 *
 * @tparam T
 */
template <typename T>
extern struct sql_com {
  /// 创建表
  void create_table(conn_ptr& in_ptr);

  /// 插入组件
  void insert(conn_ptr& in_ptr, const entt::observer& in_observer);
  void insert_id(conn_ptr& in_ptr, const entt::observer& in_observer);
  /// 更新组件
  void update(conn_ptr& in_ptr, const entt::observer& in_observer);
  void update_id(conn_ptr& in_ptr, const entt::observer& in_observer);
  /// 选择组件
  void select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle);
  void select_id(conn_ptr& in_ptr, const std::map<std::int64_t, entt::entity>& in_handle);
  /// 销毁组件
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
};

}  // namespace doodle::database_n