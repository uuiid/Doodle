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
  std::string get_column_name(const storage& s, bool include_table_name) const override;
  std::string get_table_name(const storage& s) const override;
  void set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
  void set_struct_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const override;
};

struct alias_t : public table_info_base_t {
  std::string table_name_;
  std::type_index table_type_index_{typeid(void)};

  explicit alias_t(std::string table_alias_name, std::type_index table_type_index)
      : table_name_(std::move(table_alias_name)), table_type_index_(table_type_index) {}
  virtual ~alias_t() = default;
  template <typename Table, typename ValueType>
  alias_column_info_t operator->*(ValueType Table::* column_alias) const {
    if (table_name_.empty()) throw std::runtime_error("Table name is required for alias");
    return alias_column_info_t{column_alias, table_name_};
  }
  std::string get_table_name(const storage& s) const override;
};

template <typename Table>
alias_t alias(std::string table_alias_name) {
  return alias_t{std::move(table_alias_name), typeid(Table)};
}

template <typename Table>
alias_t NEW_ALIAS() {
  return alias_t{"NEW", typeid(Table)};
}
template <typename Table>
alias_t OLD_ALIAS() {
  return alias_t{"OLD", typeid(Table)};
}

template <typename Table, typename ValueType>
alias_column_info_t new_(ValueType Table::* column_alias);
template <typename Table, typename ValueType>
alias_column_info_t old_(ValueType Table::* column_alias);
}  // namespace doodle::orm