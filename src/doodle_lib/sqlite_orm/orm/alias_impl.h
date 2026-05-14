#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/column_operations.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {
template <typename Table>
std::string alias_column_info_t<Table>::get_column_name(const storage& s, bool include_table_name) const {
  auto l_column_name = s.get_column_name<Table>(ptr_, false);
  if (include_table_name) {
    return fmt::format("{}.{}", table_alias_name_, l_column_name);
  }
  return l_column_name;
}
template <typename Table>
std::string alias_column_info_t<Table>::get_table_name(const storage& s) const {
  return table_alias_name_;
}
template <typename Table, typename ValueType>
column_operations new_(ValueType Table::* column_alias) {
  return column_operations{alias_column_info_t<Table>{column_alias, "NEW"}};
}
template <typename Table, typename ValueType>
column_operations old_(ValueType Table::* column_alias) {
  return column_operations{alias_column_info_t<Table>{column_alias, "OLD"}};
}
}  // namespace doodle::orm
