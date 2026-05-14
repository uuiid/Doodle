#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <string>

namespace doodle::orm {
template <typename Table>
std::string column_info_t<Table>::get_column_name(const storage& s, bool include_table_name) const {
  return s.get_column_name<Table>(ptr_, include_table_name);
}
template <typename Table>
std::string column_info_t<Table>::get_table_name(const storage& s) const {
  return s.get_table_name<Table>();
}
}  // namespace doodle::orm