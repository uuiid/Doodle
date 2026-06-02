#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {

template <typename Table, typename ValueType>
result_column_info_t<Table, ValueType, alias_column_info_t> new_(ValueType Table::* column_alias) {
  return result_column_info_t<Table, ValueType, alias_column_info_t>(column_alias, "NEW");
}
template <typename Table, typename ValueType>
result_column_info_t<Table, ValueType, alias_column_info_t> old_(ValueType Table::* column_alias) {
  return result_column_info_t<Table, ValueType, alias_column_info_t>(column_alias, "OLD");
}

}  // namespace doodle::orm
