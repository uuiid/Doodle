#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include "fwd.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace doodle::orm {
struct insert_t : public statement_info_base_t {
 private:
  struct insert_state_t {
    std::vector<column_info_ptr> columns_;
    bind_value_collector_t values_;
    std::int32_t batch_size_{1};
    std::shared_ptr<column_operations_base_t> wheres_;
    std::string into_table_name_;
    storage* s_{nullptr};
    std::shared_ptr<sqlite_stmt> stmt_;
  };
  friend class storage;
  friend auto insert(storage& s) -> insert_t;
  constexpr static std::int32_t g_max_batch_size_ = 100;  // SQLite的参数限制通常是999，预留一些空间给其他参数

  std::shared_ptr<insert_state_t> state_;

 public:
  insert_t() : state_(std::make_shared<insert_state_t>()) {}

  template <typename... TableColumns>
    requires((std::is_base_of_v<column_operations, std::decay_t<TableColumns>> && ...))
  insert_t set(TableColumns&&... in_columns) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      state_->columns_.push_back(in_column.get_column_info_ptr());
      in_column.collect_bind_variants(state_->values_);
    };
    (l_iter_fun(in_columns), ...);
    return *this;
  }
  template <typename T>
    requires is_object_specialization_v<std::decay_t<T>>
  insert_t values(T&& in_object) {
    using Table         = class_type_t<std::decay_t<T>>;
    auto l_table_cloums = state_->s_->template get_table_columns<Table>();

    for (const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) continue;  // 跳过主键列
      state_->columns_.push_back(std::make_shared<column_info_t>(l_column.ptr_));
      state_->values_.bind_values_.push_back(l_column.ptr_.get_value(in_object.obj_));
    }
    return *this;
  }

  template <typename T>
    requires(std::ranges::range<T>)
  insert_t set_range(T&& values) {
    if (values.empty()) return *this;  // 如果没有值，直接返回
    if (values.size() > 100)
      throw std::runtime_error("set_range中的值太多, 目前最多只支持100个值, 以避免超出SQLite的参数限制");

    using value_type = std::ranges::range_value_t<std::decay_t<T>>;
    using Table      = value_type;
    state_->values_.bind_values_.clear();
    state_->columns_.clear();
    auto l_table_cloums = state_->s_->template get_table_columns<Table>();
    for (const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) continue;
      state_->columns_.push_back(std::make_shared<column_info_t>(l_column.ptr_));
    }
    for (const auto& value : values) {
      for (const auto& l_column : l_table_cloums) {
        if (l_column.primary_key_) continue;  // 跳过主键列
        state_->values_.bind_values_.push_back(l_column.ptr_.get_value(value));
      }
    }
    state_->batch_size_ = std::clamp(static_cast<std::int32_t>(values.size()), 1, g_max_batch_size_);
    return *this;
  }
  // 重新bing range参数
  template <typename T>
    requires(std::ranges::range<T>)
  insert_t rebind_range(T&& values) {
    if (values.empty()) return *this;  // 如果没有值，直接返回
    if (values.size() > 100)
      throw std::runtime_error("rebind_range中的值太多, 目前最多只支持100个值, 以避免超出SQLite的参数限制");
    if (values.size() != state_->batch_size_) throw rebind_range_size_mismatch_exception();

    using value_type    = std::ranges::range_value_t<std::decay_t<T>>;
    using Table         = value_type;
    auto l_table_cloums = state_->s_->template get_table_columns<Table>();
    state_->values_.bind_values_.clear();
    for (const auto& value : values) {
      for (const auto& l_column : l_table_cloums) {
        if (l_column.primary_key_) continue;  // 跳过主键列
        state_->values_.bind_values_.push_back(l_column.ptr_.get_value(value));
      }
    }
    return *this;
  }

  template <typename T>
  insert_t where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    state_->wheres_          = l_condition_fun_ptr;
    return *this;
  }
  template <typename IntoTable>
  insert_t into() {
    state_->into_table_name_ = state_->s_->get_table_name<IntoTable>();
    return *this;
  }

  std::int64_t operator()();
  std::string to_sql(const storage& s, const to_sql_ctx& in_ctx) const override;
  void prepare(storage& s, const to_sql_ctx& ctx) override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
};

inline auto insert(storage& s) -> insert_t {
  insert_t l_ret{};
  l_ret.state_->s_ = &s;
  return l_ret;
}
}  // namespace doodle::orm