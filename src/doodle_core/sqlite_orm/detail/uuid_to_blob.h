//
// Created by TD on 24-9-11.
//

#pragma once

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <sqlite_orm/sqlite_orm.h>
namespace sqlite_orm {

/**
 *  First of all is a type_printer template class.
 *  It is responsible for sqlite type string representation.
 *  We want Rect to be `BLOB` so let's just derive from
 *  blob_printer. Also there are other printers: real_printer,
 *  integer_printer and text_printer.
 */
template <>
struct type_printer<boost::uuids::uuid> : public blob_printer {};

/**
 *  This is a binder class. It is used to bind c++ values to sqlite queries.
 *  Here we have to create rect binary representation and bind it as std::vector<char>.
 *  Any statement_binder specialization must have `int bind(sqlite3_stmt*, int, const T&)` function
 *  which returns bind result. Also you can call any of `sqlite3_bind_*` functions directly.
 *  More here https://www.sqlite.org/c3ref/bind_blob.html
 */
template <>
struct statement_binder<boost::uuids::uuid> {
  int bind(sqlite3_stmt* stmt, int index, const boost::uuids::uuid& value) const {
    if (!value.is_nil()) {
      return sqlite3_bind_blob(stmt, index, value.begin(), boost::uuids::uuid::static_size(), SQLITE_TRANSIENT);
    }
    return sqlite3_bind_blob(stmt, index, nullptr, 0, SQLITE_TRANSIENT);
  }
};

/**
 *  field_printer is used in `dump` and `where` functions. Here we have to create
 *  a string from mapped object.
 */
template <>
struct field_printer<boost::uuids::uuid> {
  std::string operator()(const boost::uuids::uuid& value) const { return '{' + boost::uuids::to_string(value) + '}'; }
};

/**
 *  This is a reverse operation: here we have to specify a way to transform std::vector<char> received from
 *  database to our boost::uuids::uuid object. Every `row_extractor` specialization must have `extract(sqlite3_stmt
 * *stmt, int columnIndex)` function which returns a mapped type value.
 */
template <>
struct row_extractor<boost::uuids::uuid> {
  boost::uuids::uuid extract(sqlite3_stmt* stmt, int columnIndex) const {
    boost::uuids::uuid result{};
    const auto* bytes = static_cast<const char*>(sqlite3_column_blob(stmt, columnIndex));
    if (auto len = static_cast<size_t>(sqlite3_column_bytes(stmt, columnIndex)))
      std::copy_n(bytes, len, result.begin());
    return result;
  }
};
}  // namespace sqlite_orm
