//
// Created by TD on 24-9-24.
//

#pragma once

#include <chrono>
#include <sqlite_orm/sqlite_orm.h>
namespace sqlite_orm {

template <typename Clock, typename Duration>
struct type_printer<std::chrono::time_point<Clock, Duration>>
    : std::conditional_t<std::is_floating_point_v<typename Duration::rep>, real_printer, integer_printer> {};

template <typename Clock, typename Duration>
struct statement_binder<std::chrono::time_point<Clock, Duration>> : statement_binder<typename Duration::rep> {
  int bind(sqlite3_stmt* stmt, int index, const std::chrono::time_point<Clock, Duration>& value) const {
    return statement_binder<typename Duration::rep>::bind(stmt, index, value.count());
  }
};

template <typename Clock, typename Duration>
struct field_printer<std::chrono::time_point<Clock, Duration>> : field_printer<typename Duration::rep> {
  typename Duration::rep operator()(const std::chrono::time_point<Clock, Duration>& value) const {
    return field_printer<typename Duration::rep>::operator()(value.count());
  }
};

template <typename Clock, typename Duration>
struct row_extractor<std::chrono::time_point<Clock, Duration>> : row_extractor<typename Duration::rep> {
  std::chrono::time_point<Clock, Duration> extract(sqlite3_stmt* stmt, int columnIndex) const {
    // static std::locale g_utf_8_locale{"UTF-8"};
    const auto l_value = row_extractor<typename Duration::rep>::extract(stmt, columnIndex);
    return {l_value};
  }
};
}  // namespace sqlite_orm
