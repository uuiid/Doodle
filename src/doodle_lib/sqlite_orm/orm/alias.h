#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <string>
#include <typeindex>

namespace doodle::orm {
struct column_operations;

struct alias_column_info_t : public base_column_info_t {
  table_columns_t ptr_;
  std::string table_alias_name_;

  template <typename Table, typename ValueType>
  explicit alias_column_info_t(ValueType Table::* in_ptr, std::string table_alias_name)
      : ptr_(in_ptr), table_alias_name_(std::move(table_alias_name)) {}

  // 生成 SQL 时，别名列必须包含表别名以避免歧义
  std::string get_column_name(const storage& s, const to_sql_ctx& ctx) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};
template <typename T>
inline constexpr bool is_alias_column_t_v = std::is_base_of_v<alias_column_info_t, std::remove_cvref_t<T>>;

struct alias_info_t : public table_info_base_t {
  std::string table_name_;
  std::type_index table_type_index_{typeid(void)};

  explicit alias_info_t(std::string table_alias_name, std::type_index table_type_index)
      : table_name_(std::move(table_alias_name)), table_type_index_(table_type_index) {}

  virtual ~alias_info_t() = default;

  // std::string get_table_name(const storage& s) const override;

  std::string to_sql(const storage& s, const to_sql_ctx& ctx) const override;
  void collect_bind_variants(bind_value_collector_t& bind_variants) const override;

  template <typename Table, typename ValueType>
  result_column_info_t<Table, ValueType, alias_column_info_t> operator->*(ValueType Table::* column_alias) const {
    if (table_name_.empty()) throw std::runtime_error("Table name is required for alias");
    return result_column_info_t<Table, ValueType, alias_column_info_t>(column_alias, table_name_);
  }
};

template <typename T>
inline constexpr bool is_alias_t_v = std::is_base_of_v<alias_info_t, std::remove_cvref_t<T>>;

// fts5 rank

struct rank_info_t : public base_column_info_t {
  std::string get_column_name(const storage& s, const to_sql_ctx& ctx) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};

template <typename Table>
result_column_info_t<Table, void, alias_info_t> alias(std::string table_alias_name) {
  return result_column_info_t<Table, void, alias_info_t>(table_alias_name, typeid(Table));
}
inline rank_info_t rank() { return rank_info_t{}; }

template <typename Table, typename ValueType>
result_column_info_t<Table, ValueType, alias_column_info_t> new_(ValueType Table::* column_alias);
template <typename Table, typename ValueType>
result_column_info_t<Table, ValueType, alias_column_info_t> old_(ValueType Table::* column_alias);

// fts5 any column
struct any_column_info_t : public base_column_info_t {
  table_info_base_ptr table_info_ptr_;
  explicit any_column_info_t(std::type_index in_table_index);
  std::string get_column_name(const storage& s, const to_sql_ctx& ctx) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};
template <typename Table>
auto any_column() {
  return any_column_info_t(typeid(Table));
}

}  // namespace doodle::orm