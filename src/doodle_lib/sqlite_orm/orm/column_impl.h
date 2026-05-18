#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <string>

namespace doodle::orm {
template <typename ValueType, typename Table>
table_columns_t::table_columns_t(ValueType Table::* in_ptr)
    : table_type_index_(typeid(Table)),
      any_value_(in_ptr),
      equals_([in_ptr](const table_columns_t& other) {
        if (other.any_value_.type() != typeid(ValueType Table::*)) return false;
        auto l_ptr = std::any_cast<ValueType Table::*>(other.any_value_);
        return l_ptr && l_ptr == in_ptr;
      }),
      set_value_([](const sqlite_stmt& stmt, int columnIndex, void* out_value) {
        *static_cast<ValueType*>(out_value) = stmt.get_column_value<ValueType>(columnIndex);
      }),
      set_struct_value_([in_ptr](const sqlite_stmt& stmt, int columnIndex, void* out_value) {
        using struct_type  = std::decay_t<Table>;
        auto& struct_ref   = *static_cast<struct_type*>(out_value);
        struct_ref.*in_ptr = stmt.get_column_value<ValueType>(columnIndex);
      }),
      get_bind_value_([in_ptr](const void* struct_ptr) -> bind_value_collector_t::bind_value_t {
        using struct_type = std::decay_t<Table>;
        auto& struct_ref  = *static_cast<const struct_type*>(struct_ptr);
        return bind_value_collector_t::bind_value_t{struct_ref.*in_ptr};
      })

{}

}  // namespace doodle::orm