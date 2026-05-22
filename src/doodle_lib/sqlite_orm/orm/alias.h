#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <string>
#include <typeindex>

namespace doodle::orm {
struct column_operations;

template <typename Table, typename ValueType>
struct alias_column_t {
  ValueType Table::* column_ptr_;
  std::string table_alias_name_;

  alias_column_t(ValueType Table::* in_column_ptr, std::string table_alias_name)
      : column_ptr_(in_column_ptr), table_alias_name_(std::move(table_alias_name)) {}
};
// 是 alias_column_t 的 特化
template <typename T>
struct is_alias_column_t : std::false_type {};
template <typename Table, typename ValueType>
struct is_alias_column_t<alias_column_t<Table, ValueType>> : std::true_type {};
template <typename T>
inline constexpr bool is_alias_column_t_v = is_alias_column_t<std::remove_cvref_t<T>>::value;

// 特化：class_attr_type
template <typename Table, typename ValueType>
struct class_attr_type<alias_column_t<Table, ValueType>> {
  using ptr_type    = ValueType Table::*;
  using class_type  = Table;
  using result_type = ValueType;
};

template <typename Table>
struct alias_t {
  std::string table_name_;

  alias_t(std::string table_alias_name) : table_name_(std::move(table_alias_name)) {}

  template <typename ValueType>
  alias_column_t<Table, ValueType> operator->*(ValueType Table::* column_alias) const {
    if (table_name_.empty()) throw std::runtime_error("Table name is required for alias");
    return alias_column_t<Table, ValueType>{column_alias, table_name_};
  }
};
// 是 alias_t 的 特化
template <typename T>
struct is_alias_t : std::false_type {};
template <typename Table>
struct is_alias_t<alias_t<Table>> : std::true_type {};
template <typename T>
inline constexpr bool is_alias_t_v = is_alias_t<std::remove_cvref_t<T>>::value;

struct alias_column_info_t : public base_column_info_t {
  table_columns_t ptr_;
  std::string table_alias_name_;

  template <typename Table, typename ValueType>
  explicit alias_column_info_t(ValueType Table::* in_ptr, std::string table_alias_name)
      : ptr_(in_ptr), table_alias_name_(std::move(table_alias_name)) {}

  template <typename Table, typename ValueType>
  explicit alias_column_info_t(const alias_column_t<Table, ValueType>& in_column)
      : ptr_(in_column.column_ptr_), table_alias_name_(in_column.table_alias_name_) {}

  // 生成 SQL 时，别名列必须包含表别名以避免歧义
  std::string get_column_name(const storage& s, bool include_table_name) const override;
  std::string get_table_name(const storage& s) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};

struct alias_info_t : public table_info_base_t {
  std::string table_name_;
  std::type_index table_type_index_{typeid(void)};

  explicit alias_info_t(std::string table_alias_name, std::type_index table_type_index)
      : table_name_(std::move(table_alias_name)), table_type_index_(table_type_index) {}
  template <typename Table>
  explicit alias_info_t(alias_t<Table> in_alias)
      : table_name_(std::move(in_alias.table_name_)), table_type_index_(typeid(Table)) {}
  virtual ~alias_info_t() = default;

  std::string get_table_name(const storage& s) const override;
};

// fts5 rank

struct rank_info_t : public base_column_info_t {
  std::string get_column_name(const storage& s, bool include_table_name) const override;
  std::string get_table_name(const storage& s) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};

template <typename Table>
alias_t<Table> alias(std::string table_alias_name) {
  return alias_t<Table>{std::move(table_alias_name)};
}
inline rank_info_t rank() { return rank_info_t{}; }

template <typename Table, typename ValueType>
alias_column_t<Table, ValueType> new_(ValueType Table::* column_alias);
template <typename Table, typename ValueType>
alias_column_t<Table, ValueType> old_(ValueType Table::* column_alias);
}  // namespace doodle::orm