//
// Created by TD on 24-9-24.
//

#pragma once

#include <chrono>
#include <fmt/chrono.h>
#include <fmt/format.h>
#include <spdlog/spdlog.h>
#include <sqlite_orm/sqlite_orm.h>
namespace sqlite_orm {

/**
 *  First of all is a type_printer template class.
 *  It is responsible for sqlite type string representation.
 *  We want Rect to be `BLOB` so let's just derive from
 *  blob_printer. Also there are other printers: real_printer,
 *  integer_printer and text_printer.
 */
template <typename Duration>
struct type_printer<std::chrono::zoned_time<Duration>> : public text_printer {};

/**
 *  This is a binder class. It is used to bind c++ values to sqlite queries.
 *  Here we have to create rect binary representation and bind it as std::vector<char>.
 *  Any statement_binder specialization must have `int bind(sqlite3_stmt*, int, const T&)` function
 *  which returns bind result. Also you can call any of `sqlite3_bind_*` functions directly.
 *  More here https://www.sqlite.org/c3ref/bind_blob.html
 */
template <typename Duration>
struct statement_binder<std::chrono::zoned_time<Duration>> : public statement_binder<std::string> {
  int bind(sqlite3_stmt* stmt, int index, const std::chrono::zoned_time<Duration>& value) const {
    try {
      return statement_binder<std::string>::bind(stmt, index, fmt::to_string(value.get_sys_time()));
    } catch (fmt::format_error& err) {
      SPDLOG_ERROR(err.what());
      auto l_new = std::chrono::system_clock::now();
      return statement_binder<std::string>::bind(stmt, index, fmt::to_string(l_new));
    }
  }
};

/**
 *  field_printer is used in `dump` and `where` functions. Here we have to create
 *  a string from mapped object.
 */
template <typename Duration>
struct field_printer<std::chrono::zoned_time<Duration>> : field_printer<std::string> {
  std::string operator()(const std::chrono::zoned_time<Duration>& value) const {
    return field_printer<std::string>::operator()(fmt::to_string(value.get_sys_time()));
  }
};

/**
 *  This is a reverse operation: here we have to specify a way to transform std::vector<char> received from
 *  database to our std::chrono::zoned_time<Duration> object. Every `row_extractor` specialization must
 * have `extract(sqlite3_stmt *stmt, int columnIndex)` function which returns a mapped type value.
 */
template <typename Duration>
struct row_extractor<std::chrono::zoned_time<Duration>> : row_extractor<std::string> {
  std::chrono::zoned_time<Duration> extract(sqlite3_stmt* stmt, int columnIndex) const {
    // static std::locale g_utf_8_locale{"UTF-8"};
    const auto l_str = row_extractor<std::string>::extract(stmt, columnIndex);
    std::istringstream l_istr{l_str};
    std::chrono::time_point<std::chrono::system_clock, Duration> l_value{std::chrono::system_clock::now()};
    if (l_istr >> parse("%F %T", l_value))
      ;
    else if (l_istr.clear(), l_istr.str(l_str), l_istr >> parse("%F", l_value))
      ;
    return std::chrono::zoned_time{std::chrono::current_zone(), l_value};
  }
};
}  // namespace sqlite_orm
