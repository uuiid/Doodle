//
// Created by TD on 25-3-17.
//

#pragma once
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <sqlite_orm/sqlite_orm.h>
#include <string>
#include <vector>
namespace sqlite_orm {

/**
 *  First of all is a type_printer template class.
 *  It is responsible for sqlite type string representation.
 *  We want Rect to be `BLOB` so let's just derive from
 *  blob_printer. Also there are other printers: real_printer,
 *  integer_printer and text_printer.
 */
template <>
struct type_printer<std::vector<std::string>> : public text_printer {};

/**
 *  This is a binder class. It is used to bind c++ values to sqlite queries.
 *  Here we have to create rect binary representation and bind it as std::vector<char>.
 *  Any statement_binder specialization must have `int bind(sqlite3_stmt*, int, const T&)` function
 *  which returns bind result. Also you can call any of `sqlite3_bind_*` functions directly.
 *  More here https://www.sqlite.org/c3ref/bind_blob.html
 */
template <>
struct statement_binder<std::vector<std::string>> : public statement_binder<std::string> {
  int bind(sqlite3_stmt* stmt, int index, const std::vector<std::string>& value) const {
    auto nlohmann_json = nlohmann::json{value};
    return statement_binder<std::string>::bind(stmt, index, nlohmann_json.dump());
  }
};

/**
 *  field_printer is used in `dump` and `where` functions. Here we have to create
 *  a string from mapped object.
 */
template <>
struct field_printer<std::vector<std::string>> : field_printer<std::string> {
  std::string operator()(const std::vector<std::string>& value) const {
    return field_printer<std::string>::operator()(fmt::format("{}", fmt::join(value, ",")));
  }
};

/**
 *  This is a reverse operation: here we have to specify a way to transform std::vector<char> received from
 *  database to our std::vector<std::string> object. Every `row_extractor` specialization must have
 * `extract(sqlite3_stmt *stmt, int columnIndex)` function which returns a mapped type value.
 */
template <>
struct row_extractor<std::vector<std::string>> : row_extractor<std::string> {
  std::vector<std::string> extract(sqlite3_stmt* stmt, int columnIndex) const {
    // static std::locale g_utf_8_locale{"UTF-8"};
    const auto l_str = row_extractor<std::string>::extract(stmt, columnIndex);
    std::string l_str_u8{l_str.begin(), l_str.end()};
    nlohmann::json l_json = nlohmann::json::parse(l_str);
    return l_json.get<std::vector<std::string>>();
  }
};
}  // namespace sqlite_orm