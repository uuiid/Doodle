#pragma once

#include "doodle_core/doodle_core_fwd.h"

#include <cstdint>
#include <entt/entity/fwd.hpp>

namespace doodle::database_n {

namespace detail {
template <typename... t>
struct wrong : std::false_type {};

template <typename... t>
static constexpr auto wrong_v = detail::wrong<t...>::value_type();

}  // namespace detail

/**
 * @brief 这是一个模板类, 用来特化数据库类型的
 *
 * @tparam T
 */
template <typename t>
struct sql_com {
  static_assert(detail::wrong_v<t>, "sql_com must be specialized");
  /// 创建表
  void create_table(conn_ptr& in_ptr);

  /// 插入组件
  void insert(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id);
  /// 更新组件
  void update(conn_ptr& in_ptr, const std::vector<entt::handle>& in_id);
  /// 选择组件
  void select(conn_ptr& in_ptr, const std::map<std::int64_t, entt::handle>& in_handle);
  /// 销毁组件
  void destroy(conn_ptr& in_ptr, const std::vector<std::int64_t>& in_handle);
  /// 区分更新和插入
  std::tuple<std::map<std::int64_t, entt::handle>, std::vector<entt::handle>> split_update_install(
      doodle::conn_ptr& in_ptr, const std::vector<entt::handle>& in_entts
  );
};

}  // namespace doodle::database_n