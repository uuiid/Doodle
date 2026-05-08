#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_lib/sqlite_orm/orm/fwd.h>

namespace doodle::orm {

template <typename... TableColumns>
struct select_t {
  static_assert(sizeof...(TableColumns) > 0, "至少需要选择一个列");
};
template <typename Table>
auto object() -> Table;

template <typename... TableColumns>
auto select(TableColumns... in_columns) {

};

}  // namespace doodle::orm