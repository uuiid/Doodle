#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {

template <typename... TableColumns>
auto update(TableColumns... in_columns);

struct update_base_t {
 private:
  friend class storage;
  template <typename... TableColumns>
  friend auto update(TableColumns... in_columns);

  std::vector<std::function<std::string(const storage&)>> set_clauses_;
  std::function<void(sqlite_stmt&)> bind_fun_;
  std::type_index from_table_type_index_{typeid(void)};

  where_info_t wheres_;

 public:
  template <typename T>
    requires(is_column_operations_specialization_v<T>)
  update_base_t& where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    wheres_.condition_fun_   = [l_condition_fun_ptr](const storage& s) {
      return fmt::format("WHERE {}", l_condition_fun_ptr->to_sql(s));
    };
    wheres_.bind_fun_ = [l_condition_fun_ptr](sqlite_stmt& stmt) { l_condition_fun_ptr->bind(stmt); };
    return *this;
  }
  template <typename FromTable>
  update_base_t& from() {
    from_table_type_index_ = std::type_index{typeid(FromTable)};
    return *this;
  }
};

using update_t = update_base_t;

template <typename... TableColumns>
auto update(TableColumns... in_columns) {
  static_assert(sizeof...(TableColumns) > 0, "至少需要更新一个列");
  update_t l_update{};
  auto l_iter_fun = [&l_update](auto&& column) {
    using column_or_struct_type = std::decay_t<decltype(column)>;
    if constexpr (is_column_operations_specialization_v<column_or_struct_type>) {
      auto col_ptr = std::make_shared<std::decay_t<decltype(column)>>(std::forward<decltype(column)>(column));
      l_update.set_clauses_.push_back([col = col_ptr](const storage& s) { return col->to_sql(s); });
      l_update.bind_fun_ = [col = col_ptr](sqlite_stmt& stmt) { col->bind(stmt); };
    } else {
      static_assert(always_false<column_or_struct_type>, "不支持的参数类型");
    }
  };

  (l_iter_fun(in_columns), ...);
  return l_update;
}

}  // namespace doodle::orm