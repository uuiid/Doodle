#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <optional>
#include <type_traits>
#include <utility>

namespace doodle::orm {

template <typename Column>
struct sum_value_type {
  using type = std::optional<class_result_type_t<Column>>;
};

template <typename Column>
using sum_value_type_t = typename sum_value_type<Column>::type;

struct sum_column_info_t : public base_column_info_t {
  column_info_ptr column_info_{};
  explicit sum_column_info_t(column_info_ptr in_column) : column_info_(std::move(in_column)) {}
  sum_column_info_t() = default;
  std::string get_column_name(const session& s, const to_sql_ctx& ctx) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const override;
};

template <typename T>
inline constexpr bool is_sum_t_v = std::is_base_of_v<sum_column_info_t, std::remove_cvref_t<T>>;

template <typename Value>
struct sum_set_value {
  static void apply(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value);
};

template <>
struct sum_set_value<std::optional<std::int64_t>> {
  static void apply(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value);
};

template <>
struct sum_set_value<std::optional<std::double_t>> {
  static void apply(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value);
};

template <typename Column>
result_column_info_t<void, sum_value_type_t<Column>, sum_column_info_t> sum(Column&& column) {
  return result_column_info_t<void, sum_value_type_t<Column>, sum_column_info_t>(
      sum_column_info_t{std::make_shared<column_info_t>(table_columns_t{std::forward<Column>(column)})}
  );
}
template <typename Table, typename ValueType>
result_column_info_t<void, std::optional<ValueType>, sum_column_info_t> sum(ValueType Table::* column) {
  return result_column_info_t<void, std::optional<ValueType>, sum_column_info_t>(
      sum_column_info_t{std::make_shared<column_info_t>(table_columns_t{column})}
  );
}
}  // namespace doodle::orm
