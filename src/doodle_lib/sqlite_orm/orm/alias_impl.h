#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
template <typename Table, typename ValueType>
std::string alias_column_info_t<Table, ValueType>::get_column_name(const storage& s, bool include_table_name) const {
  auto l_column_name = s.get_column_name<Table>(ptr_, false);
  // if (include_table_name) {
  return fmt::format("{}.{}", table_alias_name_, l_column_name);
  // }
  // return l_column_name;
}
template <typename Table, typename ValueType>
std::string alias_column_info_t<Table, ValueType>::get_table_name(const storage& s) const {
  return table_alias_name_;
}
template <typename Table, typename ValueType>
alias_column_info_t<Table, ValueType> new_(ValueType Table::* column_alias) {
  return alias_column_info_t<Table, ValueType>{column_alias, "NEW"};
}
template <typename Table, typename ValueType>
alias_column_info_t<Table, ValueType> old_(ValueType Table::* column_alias) {
  return alias_column_info_t<Table, ValueType>{column_alias, "OLD"};
}
}  // namespace doodle::orm
