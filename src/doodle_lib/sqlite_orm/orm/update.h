#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <functional>

namespace doodle::orm {

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

struct update_object_t {
  std::function<update_base_t(const storage&, update_object_t&)> update_fun_;

  where_info_t wheres_;
  template <typename T>
    requires(is_column_operations_specialization_v<T>)
  update_object_t& where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    wheres_.condition_fun_   = [l_condition_fun_ptr](const storage& s) {
      return fmt::format("WHERE {}", l_condition_fun_ptr->to_sql(s));
    };
    wheres_.bind_fun_ = [l_condition_fun_ptr](sqlite_stmt& stmt) { l_condition_fun_ptr->bind(stmt); };
    return *this;
  }
  void operator()(storage& s);
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

template <typename TableObject>
auto update_object(const TableObject& obj) {
  update_object_t l_update_obj{};
  using T                  = std::decay_t<TableObject>;

  l_update_obj.update_fun_ = [&obj](const storage& s, update_object_t& update_obj) {
    update_base_t l_update{};
    auto l_table_cloums = s.template get_table_columns<T>();
    column_info<T> l_primary_key_{};
    for (const auto& column : l_table_cloums) {
      if (column.primary_key_) {  // 主键不更新
        l_primary_key_ = column;
        continue;
      }
      auto col_ptr = std::make_shared<column_operations<T>>(std::forward<decltype(column.ptr_.ptr_)>(column.ptr_.ptr_));
      *col_ptr     = obj.*(column.ptr_.ptr_);

      l_update.set_clauses_.push_back([col_ptr](const storage& s) { return col_ptr->to_sql(s); });
      l_update.bind_fun_ = [col_ptr](sqlite_stmt& stmt) { col_ptr->bind(stmt); };
    }
    l_update.from<T>();
    if (!update_obj.wheres_)
      l_update.where(column_operations<T>{l_primary_key_.ptr_.ptr_} == obj.*(l_primary_key_.ptr_.ptr_));
  };

  return l_update_obj;
}

}  // namespace doodle::orm