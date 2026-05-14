#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

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

struct update_t {
 private:
  friend class storage;
  friend update_t update(storage& s);

  std::vector<std::shared_ptr<column_operations_base_t>> column_operations_;
  std::string from_table_name_;

  std::shared_ptr<column_operations_base_t> wheres_;
  storage* s_{nullptr};
  std::shared_ptr<sqlite_stmt> stmt_;

  std::vector<std::shared_ptr<storage_column_variant>> bind_variants_{};

 public:
  template <typename T>
  update_t& where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    wheres_                  = l_condition_fun_ptr;
    return *this;
  }
  template <typename FromTable>
  update_t& from() {
    from_table_name_ = s_->get_table_name<FromTable>();
    return *this;
  }

  template <typename... TableColumns>
  update_t& set(TableColumns&&... in_columns) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      if constexpr (std::is_base_of_v<column_operations, column_or_struct_type>) {
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
          auto col_ptr = std::make_shared<column_operations>(std::forward<decltype(l_column.ptr_)>(l_column.ptr_));
          *col_ptr     = in_column.obj_.*(l_column.ptr_);

          column_operations_.push_back(col_ptr);
        }
        from<Table>();
        where(column_operations{l_primary_key_.ptr_} == in_column.obj_.*(l_primary_key_.ptr_));
      } else {
        static_assert(always_false<column_or_struct_type>, "不支持的参数类型");
      }
    };

    (l_iter_fun(in_columns), ...);
    return *this;
  }

  std::string to_sql(bool in_include_table_name) const;

  update_t& operator()() &;
  update_t operator()() &&;
};

inline update_t update(storage& s) {
  update_t l_update{};
  l_update.s_ = &s;
  return l_update;
}
}  // namespace doodle::orm