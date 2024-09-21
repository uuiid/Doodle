//
// Created by TD on 24-9-21.
//

#pragma once
#include <filesystem>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
namespace sqlite_orm {

/**
 *  First of all is a type_printer template class.
 *  It is responsible for sqlite type string representation.
 *  We want Rect to be `BLOB` so let's just derive from
 *  blob_printer. Also there are other printers: real_printer,
 *  integer_printer and text_printer.
 */
template <>
struct type_printer<std::filesystem::path> : public text_printer {};

/**
 *  This is a binder class. It is used to bind c++ values to sqlite queries.
 *  Here we have to create rect binary representation and bind it as std::vector<char>.
 *  Any statement_binder specialization must have `int bind(sqlite3_stmt*, int, const T&)` function
 *  which returns bind result. Also you can call any of `sqlite3_bind_*` functions directly.
 *  More here https://www.sqlite.org/c3ref/bind_blob.html
 */
template <>
struct statement_binder<std::filesystem::path> : public statement_binder<std::string> {
  int bind(sqlite3_stmt* stmt, int index, const std::filesystem::path& value) const {
    return statement_binder<std::string>::bind(stmt, index, value.generic_string());
  }
};

/**
 *  field_printer is used in `dump` and `where` functions. Here we have to create
 *  a string from mapped object.
 */
template <>
struct field_printer<std::filesystem::path> : field_printer<std::string> {
  std::string operator()(const std::filesystem::path& value) const {
    return field_printer<std::string>::operator()(value.generic_string());
  }
};

/**
 *  This is a reverse operation: here we have to specify a way to transform std::vector<char> received from
 *  database to our std::filesystem::path object. Every `row_extractor` specialization must have `extract(sqlite3_stmt
 * *stmt, int columnIndex)` function which returns a mapped type value.
 */
template <>
struct row_extractor<std::filesystem::path> : row_extractor<std::string> {
  std::filesystem::path extract(sqlite3_stmt* stmt, int columnIndex) const {
    // static std::locale g_utf_8_locale{"UTF-8"};
    const auto l_str = row_extractor<std::string>::extract(stmt, columnIndex);
    std::basic_string<char8_t> l_str_u8{l_str.begin(), l_str.end()};
    return std::filesystem::path{l_str_u8};
  }
};
}  // namespace sqlite_orm