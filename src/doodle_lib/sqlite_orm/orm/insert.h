#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/core/core_set.h>
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/session.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

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
    std::shared_ptr<column_operations_base_t> wheres_;
    std::string into_table_name_;
    session s_{};
    std::shared_ptr<sqlite_stmt> stmt_;
    // 是一次插入多条数据吗, 如果是, 那么 values_ 中的值是多条数据的值, 需要在 to_sql 中生成 (?, ?, ?), (?, ?, ?), (?,
    // ?, ?) 这样的语句
    bool is_batch_insert_{false};
    std::int32_t batch_insert_row_count_{0};
    bool executed_{false};
  };

  friend auto insert(const session& s) -> insert_t;

  std::shared_ptr<insert_state_t> state_;
  /// 测试成员字段 uuid_id_ 是否存在，以及是否是 uuid 类型
  template <typename T, typename = void>
  struct has_uuid_id_impl : std::false_type {};

  template <typename T>
  struct has_uuid_id_impl<T, std::enable_if_t<std::is_same_v<uuid, decltype(std::declval<T>().uuid_id_)>>>
      : std::true_type {};

  template <typename T>
  static constexpr bool has_uuid_id = has_uuid_id_impl<T>::value;
  /// 测试成员字段 created_at_ 是否存在，以及是否是 chrono::system_zoned_time 类型
  template <typename T, typename = void>
  struct has_created_at_impl : std::false_type {};

  template <typename T>
  struct has_created_at_impl<
      T, std::enable_if_t<std::is_same_v<chrono::system_zoned_time, decltype(std::declval<T>().created_at_)>>>
      : std::true_type {};

  template <typename T>
  static constexpr bool has_created_at = has_created_at_impl<T>::value;
  /// 测试成员字段 updated_at_ 是否存在，以及是否是 chrono::system_zoned_time 类型
  template <typename T, typename = void>
  struct has_updated_at_impl : std::false_type {};

  template <typename T>
  struct has_updated_at_impl<
      T, std::enable_if_t<std::is_same_v<chrono::system_zoned_time, decltype(std::declval<T>().updated_at_)>>>
      : std::true_type {};

  template <typename T>
  static constexpr bool has_updated_at = has_updated_at_impl<T>::value;

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
  insert_t values(T&& in_object) {
    using Table         = std::decay_t<T>;
    auto l_table_cloums = state_->s_.template get_table_columns<Table>();
    if constexpr (has_uuid_id<Table>) {
      if (in_object.uuid_id_.is_nil()) in_object.uuid_id_ = core_set::get_set().get_uuid();
    }
    if constexpr (has_created_at<Table>)
      in_object.created_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
    if constexpr (has_updated_at<Table>)
      in_object.updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

    for (const auto& l_column : l_table_cloums) {
      if (l_column.primary_key_) continue;  // 跳过主键列
      state_->columns_.push_back(std::make_shared<column_info_t>(l_column.ptr_));
      state_->values_.bind_values_.push_back(l_column.ptr_.get_value(in_object));
    }
    return *this;
  }

  template <typename T>
    requires(std::ranges::range<T>)
  insert_t set_range(T&& values) {
    if (values.empty()) return *this;  // 如果没有值，直接返回
    state_->is_batch_insert_ = true;
    using value_type         = std::ranges::range_value_t<std::decay_t<T>>;
    using Table              = value_type;
    state_->values_.bind_values_.clear();
    state_->columns_.clear();
    auto l_table_cloums = state_->s_.template get_table_columns<Table>();
    // 循环设置
    if constexpr (has_uuid_id<Table> || has_created_at<Table> || has_updated_at<Table>)
      for (auto&& value : values) {
        if constexpr (has_uuid_id<Table>) {
          if (value.uuid_id_.is_nil()) value.uuid_id_ = core_set::get_set().get_uuid();
        }
        if constexpr (has_created_at<Table>)
          value.created_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
        if constexpr (has_updated_at<Table>)
          value.updated_at_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};
      }

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
    state_->into_table_name_ = state_->s_.get_table_name<IntoTable>();
    return *this;
  }

  std::int64_t operator()();
  std::string to_sql(const session& s, const to_sql_ctx& in_ctx) const override;
  void prepare(session& s, const to_sql_ctx& ctx) override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;
};

inline auto insert(const session& s) -> insert_t {
  insert_t l_ret{};
  l_ret.state_->s_ = s;
  return l_ret;
}
}  // namespace doodle::orm