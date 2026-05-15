#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>

#include <string>

namespace doodle::orm {
struct column_operations;
// template <typename Table, typename ValueType>
// struct column_alias_t {
//   using column_ptr_type = ValueType Table::*;
//   column_ptr_type ptr_;
//   std::string table_alias_name_;
// };
// // 检查是否是 column_alias_t<T, ValueType> 的特化
// template <typename T>
// struct is_column_alias_specialization : std::false_type {};
// template <typename Table, typename ValueType>
// struct is_column_alias_specialization<column_alias_t<Table, ValueType>> : std::true_type {};
// template <typename T>
// inline constexpr bool is_column_alias_specialization_v =
// is_column_alias_specialization<std::remove_cvref_t<T>>::value;
// 运行时别名
template <typename Table, typename ValueType>
struct alias_column_info_t : public base_column_info_t {
  using column_ptr_type = ValueType Table::*;
  using value_type      = ValueType;
  using table_type      = Table;
  table_columns_t<Table> ptr_;
  std::string table_alias_name_;

  template <typename T>
  explicit alias_column_info_t(auto T::* in_ptr, std::string table_alias_name)
      : ptr_(in_ptr), table_alias_name_(std::move(table_alias_name)) {}
  // 生成 SQL 时，别名列必须包含表别名以避免歧义
  std::string get_column_name(const storage& s, bool include_table_name) const override;
  std::string get_table_name(const storage& s) const override;
};

// 检查是否是 alias_column_info_t<T> 的特化
template <typename T>
struct is_alias_column_info_specialization : std::false_type {};
template <typename Table, typename ValueType>
struct is_alias_column_info_specialization<alias_column_info_t<Table, ValueType>> : std::true_type {};
template <typename T>
inline constexpr bool is_alias_column_info_specialization_v =
    is_alias_column_info_specialization<std::remove_cvref_t<T>>::value;

// 特化：alias_column_info_t<Table>
template <typename Table, typename ValueType>
struct class_attr_type<alias_column_info_t<Table, ValueType>> {
  using ptr_type    = ValueType;  // alias_column_info_t<Table> 不对应具体成员指针，因此使用 void 占位
  using class_type  = Table;
  using result_type = ValueType;
};

template <typename Table>
struct alias_t {
  std::string table_name_;
  using table_type = Table;
  template <typename ValueType>
  alias_column_info_t<Table, ValueType> operator->*(ValueType Table::* column_alias) const {
    if (table_name_.empty()) throw std::runtime_error("Table name is required for alias");
    return alias_column_info_t<Table, ValueType>{column_alias, table_name_};
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
alias_column_info_t<Table, ValueType> new_(ValueType Table::* column_alias);
template <typename Table, typename ValueType>
alias_column_info_t<Table, ValueType> old_(ValueType Table::* column_alias);
}  // namespace doodle::orm