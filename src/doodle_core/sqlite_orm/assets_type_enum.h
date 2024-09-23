//
// Created by TD on 24-9-23.
//

#pragma once
#include <doodle_core/metadata/assets.h>

#include <magic_enum.hpp>
#include <sqlite_orm/sqlite_orm.h>
/**
 *  This is where magic happens. To tell sqlite_orm how to act
 *  with doodle::details::assets_type_enum  enum we have to create a few service classes
 *  specializations (traits) in sqlite_orm namespace.
 */
namespace sqlite_orm {

/**
 *  First of all is a type_printer template class.
 *  It is responsible for sqlite type string representation.
 *  We want doodle::details::assets_type_enum  to be `TEXT` so let's just derive from
 *  text_printer. Also there are other printers: real_printer and
 *  integer_printer. We must use them if we want to map our type to `REAL` (double/float)
 *  or `INTEGER` (int/long/short etc) respectively.
 */
template <>
struct type_printer<doodle::details::assets_type_enum> : public text_printer {};

/**
 *  This is a binder class. It is used to bind c++ values to sqlite queries.
 *  Here we have to create gender string representation and bind it as string.
 *  Any statement_binder specialization must have `int bind(sqlite3_stmt*, int, const T&)` function
 *  which returns bind result. Also you can call any of `sqlite3_bind_*` functions directly.
 *  More here https://www.sqlite.org/c3ref/bind_blob.html
 */
template <>
struct statement_binder<doodle::details::assets_type_enum> {
  int bind(sqlite3_stmt* stmt, int index, const doodle::details::assets_type_enum& value) {
    return statement_binder<std::string_view>().bind(stmt, index, magic_enum::enum_name(value));
    //  or return sqlite3_bind_text(stmt, index++, doodle::details::assets_type_enum ToString(value).c_str(), -1,
    //  SQLITE_TRANSIENT);
  }
};

/**
 *  field_printer is used in `dump` and `where` functions. Here we have to create
 *  a string from mapped object.
 */
template <>
struct field_printer<doodle::details::assets_type_enum> {
  std::string operator()(const doodle::details::assets_type_enum& t) const {
    return std::string{magic_enum::enum_name(t)};
  }
};

/**
 *  This is a reverse operation: here we have to specify a way to transform string received from
 *  database to our doodle::details::assets_type_enum  object. Here we call `doodle::details::assets_type_enum
 * FromString` and throw `std::runtime_error` if it returns nullptr. Every `row_extractor` specialization must have
 * `extract(const char*)` and `extract(sqlite3_stmt *stmt, int columnIndex)` functions which return a mapped type value.
 */
template <>
struct row_extractor<doodle::details::assets_type_enum> {
  doodle::details::assets_type_enum extract(const char* columnText) const {
    return magic_enum::enum_cast<doodle::details::assets_type_enum>(columnText).value();
  }

  doodle::details::assets_type_enum extract(sqlite3_stmt* stmt, int columnIndex) const {
    const auto str = sqlite3_column_text(stmt, columnIndex);
    return this->extract(reinterpret_cast<const char*>(str));
  }
};
}  // namespace sqlite_orm