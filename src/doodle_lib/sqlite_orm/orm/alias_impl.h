#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/alias.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

namespace doodle::orm {

template <typename Table, typename ValueType>
alias_column_t<Table, ValueType> new_(ValueType Table::* column_alias) {
  return alias_column_t<Table, ValueType>{column_alias, "NEW"};
}
template <typename Table, typename ValueType>
alias_column_t<Table, ValueType> old_(ValueType Table::* column_alias) {
  return alias_column_t<Table, ValueType>{column_alias, "OLD"};
}


}  // namespace doodle::orm
