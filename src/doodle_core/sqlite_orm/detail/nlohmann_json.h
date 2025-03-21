//
// Created by TD on 24-12-25.
//

#pragma once
#include <nlohmann/json.hpp>
#include <sqlite_orm/sqlite_orm.h>
namespace sqlite_orm {

template <>
struct type_printer<nlohmann::json> : public text_printer {};

template <>
struct statement_binder<nlohmann::json> : public statement_binder<std::string> {
  int bind(sqlite3_stmt* stmt, int index, const nlohmann::json& value) const {
    auto l_json_str = value.dump();
    return statement_binder<std::string>::bind(stmt, index, l_json_str);
  }
};

template <>
struct field_printer<nlohmann::json> : field_printer<std::string> {
  std::string operator()(const nlohmann::json& value) const { return value.dump(); }
};

template <>
struct row_extractor<nlohmann::json> : row_extractor<std::string> {
  nlohmann::json extract(sqlite3_stmt* stmt, int columnIndex) const {
    const auto l_str = row_extractor<std::string>::extract(stmt, columnIndex);
    return l_str.empty() ? nlohmann::json{} : nlohmann::json::parse(l_str);
  }
};
}  // namespace sqlite_orm
