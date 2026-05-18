#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include "fwd.h"
#include <memory>
#include <string>
#include <vector>

namespace doodle::orm {
struct insert_t {
 private:
  friend class storage;

  friend auto insert(storage& s) -> insert_t;

  std::vector<column_info_ptr> columns_;
  bind_value_collector_t values_;
  std::int32_t batch_size_{1};
  std::shared_ptr<column_operations_base_t> wheres_;

  std::string into_table_name_;
  storage* s_{nullptr};
  std::shared_ptr<sqlite_stmt> stmt_;

 public:
  template <typename... TableColumns>
  insert_t& set(TableColumns&&... in_columns) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      if constexpr (std::is_base_of_v<column_operations, column_or_struct_type>) {
        columns_.push_back(in_column.get_column_info_ptr());
        in_column.collect_bind_variants(values_);

      } else if constexpr (is_object_specialization_v<column_or_struct_type>) {
        using Table         = column_or_struct_type;
        auto l_table_cloums = s_->template get_table_columns<Table>();
        column_info<Table> l_primary_key_{};
        for (const auto& l_column : l_table_cloums) {
          if (l_column.primary_key_) continue;  // 跳过主键列
          columns_.push_back(std::make_shared<column_info_t<Table>>(l_column.ptr_));
          values_.push_back(
              std::make_shared<storage_column_variant>(std::visit(
                  [&in_column](auto&& column_ptr) -> storage_column_variant {
                    return storage_column_variant{in_column.obj_.*(column_ptr)};
                  },
                  l_column.ptr_
              ))
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
    requires(std::ranges::range<T>)
  insert_t& set_range(T&& values) {
    if (values.empty()) return *this;  // 如果没有值，直接返回
    if (values.size() > 100)
      throw std::runtime_error("set_range中的值太多, 目前最多只支持100个值, 以避免超出SQLite的参数限制");

    using value_type    = std::decay_t<T>::value_type;
    using Table         = value_type;
    auto l_table_cloums = s_->template get_table_columns<Table>();
    for (const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) continue;
      columns_.push_back(std::make_shared<column_info_t<Table>>(l_column.ptr_));
    }
    for (const auto& value : values) {
      for (const auto& l_column : l_table_cloums) {
        if (l_column.primary_key_) continue;  // 跳过主键列
        values_.push_back(
            std::make_shared<storage_column_variant>(std::visit(
                [&value](auto&& column_ptr) -> storage_column_variant {
                  return storage_column_variant{value.*(column_ptr)};
                },
                l_column.ptr_
            ))
        );
      }
    }
    batch_size_ = static_cast<std::int32_t>(values.size());
    return *this;
  }
  // 重新bing range参数
  template <typename T>
    requires(std::ranges::range<T>)
  auto& rebind_range(T&& values) {
    if (values.empty()) return *this;  // 如果没有值，直接返回
    if (values.size() > 100)
      throw std::runtime_error("rebind_range中的值太多, 目前最多只支持100个值, 以避免超出SQLite的参数限制");
    if (values.size() != batch_size_) throw std::runtime_error("rebind_range中的值数量必须与set_range时一致");

    using value_type    = std::decay_t<T>::value_type;
    using Table         = value_type;
    auto l_table_cloums = s_->template get_table_columns<Table>();
    values_.clear();
    for (const auto& value : values) {
      for (const auto& l_column : l_table_cloums) {
        if (l_column.primary_key_) continue;  // 跳过主键列
        values_.push_back(
            std::make_shared<storage_column_variant>(std::visit(
                [&value](auto&& column_ptr) -> storage_column_variant {
                  return storage_column_variant{value.*(column_ptr)};
                },
                l_column.ptr_
            ))
        );
      }
    }
    return *this;
  }

  template <typename T>
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
  std::string to_sql(bool in_include_table_name) const;
};

inline auto insert(storage& s) -> insert_t {
  insert_t l_ret{};
  l_ret.s_ = &s;
  return l_ret;
}
}  // namespace doodle::orm