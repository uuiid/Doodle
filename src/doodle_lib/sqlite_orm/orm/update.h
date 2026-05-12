#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <functional>

namespace doodle::orm {
namespace detail {
template <typename T>
struct update_arg_type {
  using type = std::decay_t<T>;
};

template <typename C, typename T>
struct update_arg_type<T C::*> {
  using type = std::decay_t<T>;
};

template <typename Table>
struct update_arg_type<object_t<Table>> {
  using type = Table;
};

template <typename T>
using update_arg_type_t = typename update_arg_type<std::decay_t<T>>::type;

}  // namespace detail
template <typename... TableColumns>
auto update(TableColumns... in_columns);
template <typename TableObject>
auto update_object(const TableObject& obj);

struct update_base_t {
 private:
  friend class storage;
  template <typename... TableColumns>
  friend auto update(TableColumns... in_columns);
  template <typename TableObject>
  friend auto update_object(const TableObject& obj);

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

  void operator()(storage& s);
};

using update_t = update_base_t;

template <typename... TableColumns>
auto update(storage& s, TableColumns... in_columns) {
  static_assert(sizeof...(TableColumns) > 0, "至少需要更新一个列");
  update_t l_update{};
  auto l_iter_fun = [&l_update](auto&& in_column) {
    using column_or_struct_type = std::decay_t<decltype(in_column)>;
    if constexpr (is_column_operations_specialization_v<column_or_struct_type>) {
      auto col_ptr = std::make_shared<std::decay_t<decltype(in_column)>>(std::forward<decltype(in_column)>(in_column));
      l_update.set_clauses_.push_back([col = col_ptr](const storage& s) { return col->to_sql(s); });
      l_update.bind_fun_ = [col = col_ptr](sqlite_stmt& stmt) { col->bind(stmt); };
    } else if constexpr (is_object_specialization_v<column_or_struct_type>) {
      using Table         = column_or_struct_type;
      const storage* s    = in_column.s_;
      auto l_table_cloums = s->template get_table_columns<Table>();
      column_info<Table> l_primary_key_{};
      for (const auto& l_column : l_table_cloums) {
        if (l_column.primary_key_) {  // 主键不更新
          l_primary_key_ = l_column;
          continue;
        }
        auto col_ptr =
            std::make_shared<column_operations<Table>>(std::forward<decltype(l_column.ptr_.ptr_)>(l_column.ptr_.ptr_));
        *col_ptr = in_column.obj_.*(l_column.ptr_.ptr_);

        l_update.set_clauses_.push_back([col_ptr](const storage& s) { return col_ptr->to_sql(s); });
        l_update.bind_fun_ = [col_ptr](sqlite_stmt& stmt) { col_ptr->bind(stmt); };
      }
      l_update.from<Table>();
      l_update.where(column_operations<Table>{l_primary_key_.ptr_.ptr_} == in_column.obj_.*(l_primary_key_.ptr_.ptr_));
    }

    else {
      static_assert(always_false<column_or_struct_type>, "不支持的参数类型");
    }
  };

  (l_iter_fun(in_columns), ...);
  return l_update;
}
}  // namespace doodle::orm