#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

namespace doodle::orm {

struct count_column_info_t : public base_column_info_t {
  table_columns_t column_infos_;
  explicit count_column_info_t(table_columns_t count) : column_infos_(std::move(count)) {}
  count_column_info_t() = default;
  std::string get_column_name(const session& s, const to_sql_ctx& ctx) const override;
  // void set_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, const std::any& out_value) const override;
};

template <typename T>
inline constexpr bool is_count_t_v = std::is_base_of_v<count_column_info_t, std::remove_cvref_t<T>>;

template <typename Column>
result_column_info_t<void, std::int64_t, count_column_info_t> count(Column&& columns) {
  return result_column_info_t<void, std::int64_t, count_column_info_t>(
      count_column_info_t{table_columns_t{std::forward<Column>(columns)}}
  );
}
inline result_column_info_t<void, std::int64_t, count_column_info_t> count() {
  return result_column_info_t<void, std::int64_t, count_column_info_t>();
}
}  // namespace doodle::orm