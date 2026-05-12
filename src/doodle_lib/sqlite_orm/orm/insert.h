#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include "fwd.h"
#include <string>
#include <vector>

namespace doodle::orm {
struct insert_t {
 private:
  friend class storage;

  friend auto insert(storage& s) -> insert_t;

  std::vector<std::string> columns_;
  std::vector<storage_column_variant> values_;
  std::shared_ptr<column_operations_base_t> wheres_;

  std::string into_table_name_;
  storage* s_{nullptr};
  std::shared_ptr<sqlite_stmt> stmt_;

 public:
  template <typename... TableColumns>
  insert_t& set(TableColumns&&... in_columns) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      if constexpr (is_column_operations_specialization_v<column_or_struct_type>) {
        columns_.push_back(in_column.get_column_name(*s_));
        const auto& value_variants = in_column.get_value_variants();
        if (value_variants.size() != 1)
          throw std::runtime_error("Only single value is supported for column operations in insert set");

        for (const auto& v : value_variants) {
          values_.push_back(*v);
        }
      } else if constexpr (is_object_specialization_v<column_or_struct_type>) {
        using Table         = column_or_struct_type;
        auto l_table_cloums = s_->template get_table_columns<Table>();
        column_info<Table> l_primary_key_{};
        for (const auto& l_column : l_table_cloums) {
          if (l_column.primary_key_) continue;  // 跳过主键列
          columns_.push_back(l_column.ptr_.name_);
          values_.push_back(
              std::visit(
                  [&in_column](auto&& column_ptr) -> storage_column_variant {
                    return in_column.obj_.*(column_ptr);
                  },
                  l_column.ptr_.ptr_
              )
          );
        }
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
    into_table_name_ = s_->get_table_name<IntoTable>();
    return *this;
  }

  insert_t& operator()();
};

inline auto insert(storage& s) -> insert_t {
  insert_t l_ret{};
  l_ret.s_ = &s;
  return l_ret;
}
}  // namespace doodle::orm