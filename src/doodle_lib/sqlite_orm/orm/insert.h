#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <vector>

namespace doodle::orm {
struct insert_t {
 private:
  friend class storage;

  friend auto insert(storage& s) -> insert_t;

  std::vector<std::shared_ptr<column_operations_base_t>> column_operations_;
  std::shared_ptr<column_operations_base_t> wheres_;

  std::type_index into_table_type_index_{typeid(void)};
  storage* s_{nullptr};

 public:
  template <typename... TableColumns>
  insert_t& set(TableColumns&&... in_columns) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      if constexpr (is_column_operations_specialization_v<column_or_struct_type>) {
        auto col_ptr =
            std::make_shared<std::decay_t<decltype(in_column)>>(std::forward<decltype(in_column)>(in_column));
        column_operations_.push_back(col_ptr);
      } else if constexpr (is_object_specialization_v<column_or_struct_type>) {
        using Table         = column_or_struct_type;
        auto l_table_cloums = s_->template get_table_columns<Table>();
        column_info<Table> l_primary_key_{};
        for (const auto& l_column : l_table_cloums) {
          if (l_column.primary_key_) {  // 主键不更新
            l_primary_key_ = l_column;
            continue;
          }
          auto col_ptr = std::make_shared<column_operations<Table>>(
              std::forward<decltype(l_column.ptr_.ptr_)>(l_column.ptr_.ptr_)
          );
          *col_ptr = in_column.obj_.*(l_column.ptr_.ptr_);

          column_operations_.push_back(col_ptr);
        }
        from<Table>();
        where(column_operations<Table>{l_primary_key_.ptr_.ptr_} == in_column.obj_.*(l_primary_key_.ptr_.ptr_));
      } else {
        static_assert(always_false<column_or_struct_type>, "不支持的参数类型");
      }
    };
    (l_iter_fun(in_columns), ...);
    return *this;
  }
  template <typename T>
    requires(is_column_operations_specialization_v<T>)
  insert_t& where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    wheres_                  = l_condition_fun_ptr;
    return *this;
  }
  template <typename IntoTable>
  insert_t& into() {
    into_table_type_index_ = std::type_index{typeid(IntoTable)};
  }
};

inline auto insert(storage& s) -> insert_t {
  insert_t l_ret{};
  l_ret.s_ = &s;
  return l_ret;
}
}  // namespace doodle::orm