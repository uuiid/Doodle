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

template <typename Table>
void column_info_t<Table>::set_value(const sqlite_stmt& stmt, int columnIndex, void* out_value) const {
  std::visit(
      [&](auto&& column_ptr) {
        using column_type                    = std::decay_t<decltype(column_ptr)>;
        using value_type                     = class_attr_type_t<column_type>;
        *static_cast<value_type*>(out_value) = stmt.get_column_value<value_type>(columnIndex);
      },
      ptr_
  );
}
}  // namespace doodle::orm