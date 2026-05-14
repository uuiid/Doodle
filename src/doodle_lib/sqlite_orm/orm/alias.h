#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <string>

namespace doodle::orm {
template <typename Table, typename ValueType>
struct column_alias_t {
  using column_ptr_type = ValueType Table::*;
  column_ptr_type ptr_;
  std::string table_alias_name_;
};
// 检查是否是 column_alias_t<T, ValueType> 的特化
template <typename T>
struct is_column_alias_specialization : std::false_type {};
template <typename Table, typename ValueType>
struct is_column_alias_specialization<column_alias_t<Table, ValueType>> : std::true_type {};
template <typename T>
inline constexpr bool is_column_alias_specialization_v = is_column_alias_specialization<std::remove_cvref_t<T>>::value;

template <typename Table>
struct alias_t {
  std::string table_name_;
  using table_type = Table;
  template <typename ValueType>
  column_alias_t<Table, ValueType> operator->*(ValueType Table::* column_alias) const {
    if (table_name_.empty()) throw std::runtime_error("Table name is required for alias");

    return column_alias_t<Table, ValueType>{column_alias, table_name_};
  }
};

// 检查是否是 alias<T> 的特化
template <typename T>
struct is_alias_specialization : std::false_type {};
template <typename T>
struct is_alias_specialization<alias_t<T>> : std::true_type {};
template <typename T>
inline constexpr bool is_alias_specialization_v = is_alias_specialization<std::remove_cvref_t<T>>::value;

template <typename Table>
alias_t<Table> alias(std::string table_alias_name) {
  return alias_t<Table>{std::move(table_alias_name)};
}

template <typename Table>
alias_t<Table> NEW_ALIAS() {
  return alias_t<Table>{"NEW"};
}
template <typename Table>
alias_t<Table> OLD_ALIAS() {
  return alias_t<Table>{"OLD"};
}

template <typename Table, typename ValueType>
column_alias_t<Table, ValueType> new_(ValueType Table::* column_alias) {
  return column_alias_t<Table, ValueType>{column_alias, "NEW"};
}
template <typename Table, typename ValueType>
column_alias_t<Table, ValueType> old_(ValueType Table::* column_alias) {
  return column_alias_t<Table, ValueType>{column_alias, "OLD"};
}

}  // namespace doodle::orm
