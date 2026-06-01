#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

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

struct update_t : public statement_info_base_t {
 private:
  struct update_state_t {
    std::vector<std::shared_ptr<column_operations_base_t>> column_operations_;
    std::string from_table_name_;
    std::shared_ptr<column_operations_base_t> wheres_;
    storage* s_{nullptr};
    std::shared_ptr<sqlite_stmt> stmt_;
    bind_value_collector_t bind_variants_{};
  };

  friend class storage;
  friend update_t update(storage& s);

  std::shared_ptr<update_state_t> state_;

 public:
  update_t() : state_(std::make_shared<update_state_t>()) {}

  template <typename T>
  update_t where(T&& condition_fun) {
    auto l_condition_fun_ptr = std::make_shared<T>(std::forward<T>(condition_fun));
    state_->wheres_          = l_condition_fun_ptr;
    return *this;
  }
  template <typename FromTable>
  update_t from() {
    state_->from_table_name_ = state_->s_->get_table_name<FromTable>();
    return *this;
  }

  template <typename... TableColumns>
    requires((std::is_base_of_v<column_operations, std::decay_t<TableColumns>> && ...))
  update_t set(TableColumns&&... in_columns) {
    auto l_iter_fun = [this](auto&& in_column) {
      using column_or_struct_type = std::decay_t<decltype(in_column)>;
      auto col_ptr = std::make_shared<std::decay_t<decltype(in_column)>>(std::forward<decltype(in_column)>(in_column));
      state_->column_operations_.push_back(col_ptr);
    };
    (l_iter_fun(in_columns), ...);
    return *this;
  }
  template <typename T>
    requires is_object_specialization_v<std::decay_t<T>>
  update_t set(T&& in_object) {
    using Table         = class_type_t<std::decay_t<T>>;
    auto l_table_cloums = state_->s_->template get_table_columns<Table>();
    column_info l_primary_key_{};
    for (const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) {  // 主键不更新
        l_primary_key_ = l_column;
        continue;
      }
      auto col_ptr = std::make_shared<column_operations>(l_column.ptr_);
      *col_ptr     = l_column.ptr_.get_value(in_object.obj_);

      state_->column_operations_.push_back(col_ptr);
    }
    from<Table>();
    where(column_operations{l_primary_key_.ptr_} == l_primary_key_.ptr_.get_value(in_object.obj_));
    return *this;
  }

  template <typename T>
    requires is_object_specialization_v<std::decay_t<T>>
  update_t rebind(T&& in_object) {
    using Table         = class_type_t<std::decay_t<T>>;
    auto l_table_cloums = state_->s_->template get_table_columns<Table>();
    if (l_table_cloums.size() != state_->bind_variants_.bind_values_.size())
      throw std::runtime_error("列数量与绑定变量数量不匹配，无法使用 re_set 更新");

    column_info l_primary_key_{};
    for (auto l_i = 0; const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) {  // 主键不更新
        l_primary_key_ = l_column;
        continue;
      }
      auto col_ptr                               = std::make_shared<column_operations>(l_column.ptr_);
      state_->bind_variants_.bind_values_[l_i++] = l_column.ptr_.get_value(in_object.obj_);
    }
    state_->bind_variants_.bind_values_.back() = l_primary_key_.ptr_.get_value(in_object.obj_);
    return *this;
  }

  void prepare(storage& s, const to_sql_ctx& ctx) override;

  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
  std::string to_sql(const storage& s, const to_sql_ctx& ctx) const override;

  update_t operator()();
};

inline update_t update(storage& s) {
  update_t l_update{};
  l_update.state_->s_ = &s;
  return l_update;
}
}  // namespace doodle::orm