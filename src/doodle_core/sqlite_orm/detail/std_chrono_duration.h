//
// Created by TD on 24-9-24.
//

#pragma once

#include <chrono>
#include <sqlite_orm/sqlite_orm.h>
namespace sqlite_orm {

template <typename Rep, typename Period>
struct type_printer<std::chrono::duration<Rep, Period>>
    : std::conditional_t<std::is_floating_point_v<Rep>, real_printer, integer_printer> {};

template <typename Rep, typename Period>
struct statement_binder<std::chrono::duration<Rep, Period>> : statement_binder<Rep> {
  int bind(sqlite3_stmt* stmt, int index, const std::chrono::duration<Rep, Period>& value) const {
    return statement_binder<Rep>::bind(stmt, index, value.count());
  }
};

template <typename Rep, typename Period>
struct field_printer<std::chrono::duration<Rep, Period>> : field_printer<Rep> {
  auto operator()(const std::chrono::duration<Rep, Period>& value) const {
    return field_printer<Rep>::operator()(value.count());
  }
};

template <typename Rep, typename Period>
struct row_extractor<std::chrono::duration<Rep, Period>> : row_extractor<Rep> {
  std::chrono::duration<Rep, Period> extract(sqlite3_stmt* stmt, int columnIndex) const {
    // static std::locale g_utf_8_locale{"UTF-8"};
    const auto l_value = row_extractor<Rep>::extract(stmt, columnIndex);
    return {l_value};
  }
};

}  // namespace sqlite_orm