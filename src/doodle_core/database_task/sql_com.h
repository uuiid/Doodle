#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <entt/entity/fwd.hpp>

namespace doodle::database_n {
/**
 * @brief 这是一个模板类, 用来特化数据库类型的
 *
 * @tparam T
 */
template <typename T>
struct sql_com {
  void create_table(conn_ptr& in_ptr);
  void install(conn_ptr& in_ptr, const entt::handle& in_handle);
  void update(conn_ptr& in_ptr, const entt::handle& in_handle);
  void select(conn_ptr& in_ptr, entt::handle& in_handle);
  void destroy(conn_ptr& in_ptr, const entt::handle& in_handle);
};

}  // namespace doodle::database_n