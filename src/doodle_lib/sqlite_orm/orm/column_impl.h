#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/column.h>
#include <doodle_lib/sqlite_orm/orm/fwd.h>
#include <doodle_lib/sqlite_orm/orm/storage.h>

#include <fmt/format.h>
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
      // set_value_([](const sqlite_stmt& stmt, int columnIndex, std::any out_value) {
      //   if (stmt.column_is_null(columnIndex)) return;  // 如果是NULL，不设置值，保持out_value的原值不变
      //   if (!out_value.has_value()) throw std::runtime_error("Output value is not initialized");
      //   if (out_value.type() != typeid(ValueType*))
      //     throw std::runtime_error(
      //         fmt::format(
      //             "Output value type mismatch, expected {}, got {}", typeid(ValueType*).name(), out_value.type().name()
      //         )
      //     );

      //   *std::any_cast<ValueType*>(out_value) = stmt.get_column_value<ValueType>(columnIndex);
      // }),
      set_struct_value_([in_ptr](const sqlite_stmt& stmt, int columnIndex, std::any out_value) {
        if (stmt.column_is_null(columnIndex)) return;  // 如果是NULL，不设置值，保持struct_ref的原值不变
        if (!out_value.has_value()) throw std::runtime_error("Output struct pointer is not initialized");
        using struct_type = std::decay_t<Table>;
        if (out_value.type() == typeid(ValueType*)) {
          *std::any_cast<ValueType*>(out_value) = stmt.get_column_value<ValueType>(columnIndex);
          return;
        }
        if (out_value.type() != typeid(struct_type*))
          throw std::runtime_error(
              fmt::format(
                  "Output struct pointer type mismatch, expected {}, got {}", typeid(struct_type*).name(),
                  out_value.type().name()
              )
          );

        auto& struct_ref   = *std::any_cast<struct_type*>(out_value);
        struct_ref.*in_ptr = stmt.get_column_value<ValueType>(columnIndex);
      }),
      get_bind_value_([in_ptr](const std::any& struct_ptr) -> bind_value_t {
        if (!struct_ptr.has_value()) throw std::runtime_error("Struct pointer is not initialized");
        using struct_type = std::decay_t<Table>;
        if (struct_ptr.type() != typeid(const struct_type*))
          throw std::runtime_error(
              fmt::format(
                  "Struct pointer type mismatch, expected {}, got {}", typeid(const struct_type*).name(),
                  struct_ptr.type().name()
              )
          );
        auto& struct_ref = *std::any_cast<const struct_type*>(struct_ptr);
        return bind_value_t{struct_ref.*in_ptr};
      })

{}

}  // namespace doodle::orm