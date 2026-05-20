#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <vector>

namespace doodle::orm {

template <typename Column>
struct count_t {
  static constexpr std::size_t value = 1;
  std::decay_t<Column> columns_tuple_;
  explicit count_t(std::decay_t<Column> columns_tuple) : columns_tuple_(std::move(columns_tuple)) {}
};
// void count()
template <>
struct count_t<void> {
  static constexpr std::size_t value = 0;
  count_t()                          = default;
};

// 是一个 count_t 的模板特化
template <typename Column>
struct is_count_t : std::false_type {};
template <typename Column>
struct is_count_t<count_t<Column>> : std::true_type {};
template <typename T>
inline constexpr bool is_count_t_v = is_count_t<std::decay_t<T>>::value;

// struct class_attr_type count_t 的模板特化
template <typename Column>
struct class_attr_type<count_t<Column>> {
  using ptr_type    = std::int64_t;
  using class_type  = void;  // count_t 不对应具体类类型，因此使用 void 占位
  using result_type = std::int64_t;
};

struct count_column_info_t : public base_column_info_t {
  table_columns_t column_infos_;
  template <typename Column>
  explicit count_column_info_t(count_t<Column>&& count) {
    column_infos_ = table_columns_t(count.columns_tuple_);
  }
  explicit count_column_info_t(count_t<void>&&) {}  // 处理 count() 的情况, 没有列信息
  std::string get_column_name(const storage& s, bool include_table_name) const override;
  std::string get_table_name(const storage& s) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};

template <typename Column>
count_t<Column> count(Column&& columns) {
  return count_t<Column>(std::make_tuple(std::forward<Column>(columns)));
}
inline count_t<void> count() { return count_t<void>(); }
}  // namespace doodle::orm